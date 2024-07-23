#include "justly/SongEditor.hpp"

#include <QAbstractItemModel>
#include <QAction>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGuiApplication>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaType>
#include <QRect>
#include <QScreen>
#include <QSize>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QTextStream>
#include <QThread>
#include <QUndoStack>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <vector>

#include "changes/DoubleChange.hpp"
#include "changes/InstrumentChange.hpp"
#include "justly/ChangeId.hpp"
#include "justly/Chord.hpp"
#include "justly/ChordsModel.hpp"
#include "justly/ChordsView.hpp"
#include "justly/Instrument.hpp"
#include "justly/InstrumentEditor.hpp"
#include "justly/Interval.hpp"
#include "justly/Note.hpp"
#include "justly/Rational.hpp"
#include "justly/TreeLevel.hpp"
#include "other/private.hpp"

const auto PERCENT = 100;
const auto MAX_GAIN = 10;

const auto MIN_STARTING_KEY = 60;
const auto MAX_STARTING_KEY = 440;

const auto MAX_STARTING_VOLUME = 100;

const auto MIN_STARTING_TEMPO = 25;
const auto MAX_STARTING_TEMPO = 200;

const auto CONCERT_A_FREQUENCY = 440;
const auto CONCERT_A_MIDI = 69;
const auto HALFSTEPS_PER_OCTAVE = 12;
const auto MAX_VELOCITY = 127;
const unsigned int MILLISECONDS_PER_SECOND = 1000;
const auto BEND_PER_HALFSTEP = 4096;
const auto ZERO_BEND_HALFSTEPS = 2;
// insert end buffer at the end of songs
const unsigned int START_END_MILLISECONDS = 500;
const auto VERBOSE_FLUIDSYNTH = false;
const auto SECONDS_PER_MINUTE = 60;
const auto DEFAULT_GAIN = 5;

auto get_default_driver() -> std::string {
#if defined(__linux__)
  return "pulseaudio";
#elif defined(_WIN32)
  return "wasapi";
#elif defined(__APPLE__)
  return "coreaudio";
#else
  return {};
#endif
}

auto get_settings_pointer() -> fluid_settings_t * {
  fluid_settings_t *settings_pointer = new_fluid_settings();
  Q_ASSERT(settings_pointer != nullptr);
  auto cores = std::thread::hardware_concurrency();
  if (cores > 0) {
    fluid_settings_setint(settings_pointer, "synth.cpu-cores",
                          static_cast<int>(cores));
  }
  fluid_settings_setnum(settings_pointer, "synth.gain", DEFAULT_GAIN);
  if (VERBOSE_FLUIDSYNTH) {
    fluid_settings_setint(settings_pointer, "synth.verbose", 1);
  }
  return settings_pointer;
}

auto get_soundfont_id(fluid_synth_t *synth_pointer) -> int {
  auto soundfont_file = QDir(QCoreApplication::applicationDirPath())
                            .filePath("../share/MuseScore_General.sf2")
                            .toStdString();
  Q_ASSERT(std::filesystem::exists(soundfont_file));

  auto maybe_soundfont_id =
      fluid_synth_sfload(synth_pointer, soundfont_file.c_str(), 1);
  Q_ASSERT(maybe_soundfont_id != -1);
  return maybe_soundfont_id;
}

void register_converters() {
  QMetaType::registerConverter<Rational, QString>(&Rational::text);
  QMetaType::registerConverter<Interval, QString>(&Interval::text);
  QMetaType::registerConverter<const Instrument *, QString>(
      [](const Instrument *instrument_pointer) {
        Q_ASSERT(instrument_pointer != nullptr);
        return QString::fromStdString(instrument_pointer->instrument_name);
      });
}

auto SongEditor::beat_time() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}

void SongEditor::set_playback_volume(float new_value) const {
  Q_ASSERT(synth_pointer != nullptr);
  fluid_synth_set_gain(synth_pointer, new_value);
}

void SongEditor::start_real_time(const std::string &driver) {
  delete_audio_driver();

  Q_ASSERT(settings_pointer != nullptr);
  fluid_settings_setint(settings_pointer, "synth.lock-memory", 1);
  fluid_settings_setstr(settings_pointer, "audio.driver", driver.c_str());

  Q_ASSERT(synth_pointer != nullptr);
#ifndef DISABLE_AUDIO
  audio_driver_pointer =
      new_fluid_audio_driver(settings_pointer, synth_pointer);

  if (audio_driver_pointer == nullptr) {
    QString message;
    QTextStream stream(&message);
    stream << tr("Cannot start audio driver ")
           << QString::fromStdString(driver);
    QMessageBox::warning(this, tr("Audio driver error"), message);
  }
#endif
}

void SongEditor::initialize_play() {
  Q_ASSERT(starting_key_editor_pointer != nullptr);
  current_key = starting_key_editor_pointer->value();

  Q_ASSERT(starting_volume_percent_editor_pointer != nullptr);
  current_volume = starting_volume_percent_editor_pointer->value() / PERCENT;

  Q_ASSERT(starting_tempo_editor_pointer != nullptr);
  current_tempo = starting_tempo_editor_pointer->value();

  Q_ASSERT(current_instrument_pointer != nullptr);
  current_instrument_pointer = starting_instrument_editor_pointer->value();

  Q_ASSERT(sequencer_pointer != nullptr);
  starting_time = fluid_sequencer_get_tick(sequencer_pointer);
  current_time = starting_time;

  for (size_t index = 0; index < NUMBER_OF_MIDI_CHANNELS; index = index + 1) {
    Q_ASSERT(index < channel_schedules.size());
    channel_schedules[index] = current_time;
  }
}

void SongEditor::modulate(const Chord &chord) {
  current_key = current_key * chord.interval.ratio();
  current_volume = current_volume * chord.volume_ratio.ratio();
  current_tempo = current_tempo * chord.tempo_ratio.ratio();
  const auto *chord_instrument_pointer = chord.instrument_pointer;
  Q_ASSERT(chord_instrument_pointer != nullptr);
  if (!chord_instrument_pointer->instrument_name.empty()) {
    current_instrument_pointer = chord_instrument_pointer;
  }
}

auto SongEditor::play_notes(size_t chord_index, const Chord &chord,
                            size_t first_note_index,
                            size_t number_of_notes) -> double {
  auto final_time = 0.0;
  for (auto note_index = first_note_index;
       note_index < first_note_index + number_of_notes;
       note_index = note_index + 1) {
    const auto &note = chord.get_const_note(note_index);

    const auto *note_instrument_pointer = note.instrument_pointer;

    Q_ASSERT(note_instrument_pointer != nullptr);
    const auto &instrument_pointer =
        (note_instrument_pointer->instrument_name.empty()
             ? current_instrument_pointer
             : note_instrument_pointer);

    Q_ASSERT(CONCERT_A_FREQUENCY != 0);
    auto key_float =
        HALFSTEPS_PER_OCTAVE *
            log2(current_key * note.interval.ratio() / CONCERT_A_FREQUENCY) +
        CONCERT_A_MIDI;
    auto closest_key = round(key_float);
    auto int_closest_key = static_cast<int16_t>(closest_key);

    auto channel_number = -1;
    for (size_t channel_index = 0; channel_index < NUMBER_OF_MIDI_CHANNELS;
         channel_index = channel_index + 1) {
      Q_ASSERT(channel_index < channel_schedules.size());
      if (current_time >= channel_schedules[channel_index]) {
        channel_number = static_cast<int>(channel_index);
        break;
      }
    }

    if (channel_number == -1) {
      QString message;
      QTextStream stream(&message);
      stream << tr("Out of MIDI channels for chord ") << chord_index + 1
             << tr(", note ") << note_index + 1 << tr(". Not playing note.");
      QMessageBox::warning(this, tr("MIDI channel error"), message);
    } else {
      auto int_current_time = to_unsigned(static_cast<int>(current_time));

      Q_ASSERT(instrument_pointer != nullptr);
      Q_ASSERT(event_pointer != nullptr);
      fluid_event_program_select(event_pointer, channel_number, soundfont_id,
                                 instrument_pointer->bank_number,
                                 instrument_pointer->preset_number);

      Q_ASSERT(sequencer_pointer != nullptr);
      fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                              int_current_time, 1);

      fluid_event_pitch_bend(
          event_pointer, channel_number,
          static_cast<int>((key_float - closest_key + ZERO_BEND_HALFSTEPS) *
                           BEND_PER_HALFSTEP));

      fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                              int_current_time + 1, 1);

      auto new_volume = current_volume * note.volume_ratio.ratio();
      if (new_volume > 1) {
        QString message;
        QTextStream stream(&message);
        stream << tr("Volume exceeds 100% for chord ") << chord_index + 1
               << tr(", note ") << note_index + 1
               << tr(". Playing with 100% volume.");
        QMessageBox::warning(this, tr("Volume error"), message);
        new_volume = 1;
      }

      fluid_event_noteon(event_pointer, channel_number, int_closest_key,
                         static_cast<int16_t>(new_volume * MAX_VELOCITY));
      fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                              int_current_time + 2, 1);

      auto end_time = current_time + (beat_time() * note.beats.ratio() *
                                      note.tempo_ratio.ratio()) *
                                         MILLISECONDS_PER_SECOND;

      fluid_event_noteoff(event_pointer, channel_number, int_closest_key);
      fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                              to_unsigned(static_cast<int>(end_time)), 1);
      Q_ASSERT(to_size_t(channel_number) < channel_schedules.size());
      channel_schedules[channel_number] = end_time;

      if (end_time > final_time) {
        final_time = end_time;
      }
    }
  }
  return final_time;
}

auto SongEditor::play_chords(size_t first_chord_number, size_t number_of_chords,
                             int wait_frames) -> double {
  Q_ASSERT(chords_view_pointer != nullptr);
  auto *chords_model_pointer = chords_view_pointer->chords_model_pointer;
  Q_ASSERT(chords_model_pointer != nullptr);

  current_time = current_time + wait_frames;
  auto final_time = 0.0;
  for (auto chord_index = first_chord_number;
       chord_index < first_chord_number + number_of_chords;
       chord_index = chord_index + 1) {
    const auto &chord = chords_model_pointer->get_const_chord(chord_index);

    modulate(chord);
    auto end_time = play_notes(chord_index, chord, 0, chord.notes.size());
    if (end_time > final_time) {
      final_time = end_time;
    }
    current_time = current_time + (beat_time() * chord.beats.ratio()) *
                                      MILLISECONDS_PER_SECOND;
  }
  return final_time;
}

void SongEditor::stop_playing() const {
  Q_ASSERT(sequencer_pointer != nullptr);
  fluid_sequencer_remove_events(sequencer_pointer, -1, -1, -1);

  for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
       channel_number = channel_number + 1) {
    Q_ASSERT(event_pointer != nullptr);
    fluid_event_all_sounds_off(event_pointer, channel_number);

    Q_ASSERT(sequencer_pointer != nullptr);
    fluid_sequencer_send_now(sequencer_pointer, event_pointer);
  }
}

void SongEditor::delete_audio_driver() const {
  if (audio_driver_pointer != nullptr) {
    delete_fluid_audio_driver(audio_driver_pointer);
  }
}

void SongEditor::update_actions() const {
  Q_ASSERT(chords_view_pointer != nullptr);

  auto *chords_model_pointer = chords_view_pointer->model();
  Q_ASSERT(chords_model_pointer != nullptr);

  auto *selection_model = chords_view_pointer->selectionModel();
  Q_ASSERT(selection_model != nullptr);

  auto anything_selected = selection_model->hasSelection();

  auto selected_row_indexes = selection_model->selectedRows();
  auto any_rows_selected = !selected_row_indexes.empty();
  auto can_contain = selected_row_indexes.size() == 1
                         ? get_level(selected_row_indexes[0]) == chord_level
                         : chords_model_pointer->rowCount(QModelIndex()) == 0;

  Q_ASSERT(copy_action_pointer != nullptr);
  copy_action_pointer->setEnabled(anything_selected);

  Q_ASSERT(insert_after_action_pointer != nullptr);
  insert_after_action_pointer->setEnabled(any_rows_selected);

  Q_ASSERT(delete_action_pointer != nullptr);
  delete_action_pointer->setEnabled(anything_selected);

  Q_ASSERT(play_action_pointer != nullptr);
  play_action_pointer->setEnabled(any_rows_selected);

  Q_ASSERT(paste_cells_or_after_action_pointer != nullptr);
  paste_cells_or_after_action_pointer->setEnabled(anything_selected);

  Q_ASSERT(insert_into_action_pointer != nullptr);
  insert_into_action_pointer->setEnabled(can_contain);

  Q_ASSERT(paste_into_action_pointer != nullptr);
  paste_into_action_pointer->setEnabled(can_contain);
}

SongEditor::SongEditor(QWidget *parent_pointer, Qt::WindowFlags flags)
    : QMainWindow(parent_pointer, flags) {
  auto *controls_pointer = std::make_unique<QFrame>(this).release();
  controls_pointer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QDockWidget *dock_widget_pointer =
      std::make_unique<QDockWidget>("Controls", this).release();

  auto *menu_bar_pointer = menuBar();
  Q_ASSERT(menu_bar_pointer != nullptr);

  auto *file_menu_pointer =
      std::make_unique<QMenu>(tr("&File"), this).release();

  auto *open_action_pointer =
      std::make_unique<QAction>(tr("&Open"), file_menu_pointer).release();
  file_menu_pointer->addAction(open_action_pointer);
  connect(open_action_pointer, &QAction::triggered, this, [this]() {
    Q_ASSERT(undo_stack_pointer != nullptr);
    if (undo_stack_pointer->isClean() ||
        QMessageBox::question(this, tr("Unsaved changes"),
                              tr("Discard unsaved changes?")) ==
            QMessageBox::Yes) {
      QFileDialog dialog(this, "Open — Justly", current_folder,
                         "JSON file (*.json)");

      dialog.setAcceptMode(QFileDialog::AcceptOpen);
      dialog.setDefaultSuffix(".json");
      dialog.setFileMode(QFileDialog::ExistingFile);

      if (dialog.exec() != 0) {
        current_folder = dialog.directory().absolutePath();
        const auto &selected_files = dialog.selectedFiles();
        Q_ASSERT(!(selected_files.empty()));
        open_file(selected_files[0]);
      }
    }
  });
  open_action_pointer->setShortcuts(QKeySequence::Open);

  file_menu_pointer->addSeparator();

  save_action_pointer->setShortcuts(QKeySequence::Save);
  connect(save_action_pointer, &QAction::triggered, this,
          [this]() { save_as_file(current_file); });
  file_menu_pointer->addAction(save_action_pointer);
  save_action_pointer->setEnabled(false);

  auto *save_as_action_pointer =
      std::make_unique<QAction>(tr("&Save As..."), file_menu_pointer).release();
  save_as_action_pointer->setShortcuts(QKeySequence::SaveAs);
  connect(save_as_action_pointer, &QAction::triggered, this, [this]() {
    QFileDialog dialog(this, "Save As — Justly", current_folder,
                       "JSON file (*.json)");

    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix(".json");
    dialog.setFileMode(QFileDialog::AnyFile);

    if (dialog.exec() != 0) {
      current_folder = dialog.directory().absolutePath();
      const auto &selected_files = dialog.selectedFiles();
      Q_ASSERT(!(selected_files.empty()));
      save_as_file(selected_files[0]);
    }
  });
  file_menu_pointer->addAction(save_as_action_pointer);
  save_as_action_pointer->setEnabled(true);

  auto *export_as_action_pointer =
      std::make_unique<QAction>(tr("&Export recording"), file_menu_pointer)
          .release();
  connect(export_as_action_pointer, &QAction::triggered, this, [this]() {
    QFileDialog dialog(this, "Export — Justly", current_folder,
                       "WAV file (*.wav)");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix(".wav");
    dialog.setFileMode(QFileDialog::AnyFile);

    dialog.setLabelText(QFileDialog::Accept, "Export");

    if (dialog.exec() != 0) {
      current_folder = dialog.directory().absolutePath();
      const auto &selected_files = dialog.selectedFiles();
      Q_ASSERT(!(selected_files.empty()));
      export_to_file(selected_files[0]);
    }
  });
  file_menu_pointer->addAction(export_as_action_pointer);

  menu_bar_pointer->addMenu(file_menu_pointer);

  auto *edit_menu_pointer =
      std::make_unique<QMenu>(tr("&Edit"), this).release();

  auto *undo_action_pointer =
      undo_stack_pointer->createUndoAction(edit_menu_pointer);
  undo_action_pointer->setShortcuts(QKeySequence::Undo);
  edit_menu_pointer->addAction(undo_action_pointer);

  auto *redo_action_pointer =
      undo_stack_pointer->createRedoAction(edit_menu_pointer);
  redo_action_pointer->setShortcuts(QKeySequence::Redo);
  edit_menu_pointer->addAction(redo_action_pointer);

  edit_menu_pointer->addSeparator();

  copy_action_pointer->setEnabled(false);
  copy_action_pointer->setShortcuts(QKeySequence::Copy);
  connect(copy_action_pointer, &QAction::triggered, chords_view_pointer,
          &ChordsView::copy_selected);
  edit_menu_pointer->addAction(copy_action_pointer);

  auto *paste_menu_pointer =
      std::make_unique<QMenu>(tr("&Paste"), edit_menu_pointer).release();

  paste_cells_or_after_action_pointer->setEnabled(false);
  connect(paste_cells_or_after_action_pointer, &QAction::triggered,
          chords_view_pointer, &ChordsView::paste_cells_or_after);
  paste_cells_or_after_action_pointer->setShortcuts(QKeySequence::Paste);
  paste_menu_pointer->addAction(paste_cells_or_after_action_pointer);

  paste_into_action_pointer->setEnabled(false);
  connect(paste_into_action_pointer, &QAction::triggered, chords_view_pointer,
          &ChordsView::paste_into);
  paste_menu_pointer->addAction(paste_into_action_pointer);

  edit_menu_pointer->addMenu(paste_menu_pointer);

  edit_menu_pointer->addSeparator();

  auto *insert_menu_pointer =
      std::make_unique<QMenu>(tr("&Insert"), edit_menu_pointer).release();

  edit_menu_pointer->addMenu(insert_menu_pointer);

  insert_after_action_pointer->setEnabled(false);
  insert_after_action_pointer->setShortcuts(QKeySequence::InsertLineSeparator);
  connect(insert_after_action_pointer, &QAction::triggered, chords_view_pointer,
          &ChordsView::insert_after);
  insert_menu_pointer->addAction(insert_after_action_pointer);

  insert_into_action_pointer->setEnabled(true);
  insert_into_action_pointer->setShortcuts(QKeySequence::AddTab);
  connect(insert_into_action_pointer, &QAction::triggered, chords_view_pointer,
          &ChordsView::insert_into);
  insert_menu_pointer->addAction(insert_into_action_pointer);

  delete_action_pointer->setEnabled(false);
  delete_action_pointer->setShortcuts(QKeySequence::Delete);
  connect(delete_action_pointer, &QAction::triggered, chords_view_pointer,
          &ChordsView::delete_selected);
  edit_menu_pointer->addAction(delete_action_pointer);

  menu_bar_pointer->addMenu(edit_menu_pointer);

  auto *view_menu_pointer =
      std::make_unique<QMenu>(tr("&View"), this).release();

  auto *view_controls_checkbox_pointer =
      std::make_unique<QAction>(tr("&Controls"), view_menu_pointer).release();

  view_controls_checkbox_pointer->setCheckable(true);
  view_controls_checkbox_pointer->setChecked(true);
  connect(view_controls_checkbox_pointer, &QAction::toggled,
          dock_widget_pointer, &QWidget::setVisible);
  view_menu_pointer->addAction(view_controls_checkbox_pointer);

  menu_bar_pointer->addMenu(view_menu_pointer);

  auto *play_menu_pointer =
      std::make_unique<QMenu>(tr("&Play"), this).release();

  play_action_pointer->setEnabled(false);
  play_action_pointer->setShortcuts(QKeySequence::Print);
  connect(play_action_pointer, &QAction::triggered, this, [this]() {
    Q_ASSERT(chords_view_pointer != nullptr);
    auto *selection_model = chords_view_pointer->selectionModel();
    Q_ASSERT(selection_model != nullptr);
    auto selected_row_indexes = selection_model->selectedRows();

    Q_ASSERT(!(selected_row_indexes.empty()));
    auto first_index = selected_row_indexes[0];

    auto *chords_model_pointer = chords_view_pointer->chords_model_pointer;
    Q_ASSERT(chords_model_pointer != nullptr);

    auto parent_index = chords_model_pointer->parent(first_index);

    auto first_child_number = to_size_t(first_index.row());
    auto number_of_children = selected_row_indexes.size();

    stop_playing();
    initialize_play();
    if (get_level(parent_index) == root_level) {
      for (size_t chord_index = 0; chord_index < first_child_number;
           chord_index = chord_index + 1) {
        modulate(chords_model_pointer->get_const_chord(chord_index));
      }
      play_chords(first_child_number, number_of_children);
    } else {
      auto chord_number = to_size_t(parent_index.row());
      for (size_t chord_index = 0; chord_index < chord_number;
           chord_index = chord_index + 1) {
        modulate(chords_model_pointer->get_const_chord(chord_index));
      }
      const auto &chord = chords_model_pointer->get_const_chord(chord_number);
      modulate(chord);
      play_notes(chord_number, chord, first_child_number, number_of_children);
    }
  });
  play_menu_pointer->addAction(play_action_pointer);

  stop_playing_action_pointer->setEnabled(true);
  play_menu_pointer->addAction(stop_playing_action_pointer);
  connect(stop_playing_action_pointer, &QAction::triggered, this,
          &SongEditor::stop_playing);
  stop_playing_action_pointer->setShortcuts(QKeySequence::Cancel);

  menu_bar_pointer->addMenu(play_menu_pointer);

  auto *controls_form_pointer =
      std::make_unique<QFormLayout>(controls_pointer).release();

  playback_volume_editor_pointer->setMinimum(0);
  playback_volume_editor_pointer->setMaximum(PERCENT);
  playback_volume_editor_pointer->setValue(
      static_cast<int>(get_playback_volume() / MAX_GAIN * PERCENT));
  connect(playback_volume_editor_pointer, &QSlider::valueChanged, this,
          [this](int new_value) {
            set_playback_volume(
                static_cast<float>(1.0 * new_value / PERCENT * MAX_GAIN));
          });
  controls_form_pointer->addRow(tr("&Playback volume:"),
                                playback_volume_editor_pointer);

  starting_instrument_editor_pointer->setValue(starting_instrument_pointer);
  starting_instrument_pointer = starting_instrument_editor_pointer->value();
  connect(starting_instrument_editor_pointer, &QComboBox::currentIndexChanged,
          this, [this](int new_index) {
            Q_ASSERT(undo_stack_pointer != nullptr);
            undo_stack_pointer->push(std::make_unique<InstrumentChange>(
                                         this, starting_instrument_pointer,
                                         &get_all_instruments()[new_index])
                                         .release());
          });
  controls_form_pointer->addRow(tr("Starting &instrument:"),
                                starting_instrument_editor_pointer);

  starting_key_editor_pointer->setMinimum(MIN_STARTING_KEY);
  starting_key_editor_pointer->setMaximum(MAX_STARTING_KEY);
  starting_key_editor_pointer->setDecimals(1);
  starting_key_editor_pointer->setSuffix(" hz");

  starting_key_editor_pointer->setValue(starting_key);

  connect(starting_key_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](double new_value) {
            Q_ASSERT(undo_stack_pointer != nullptr);
            undo_stack_pointer->push(
                std::make_unique<DoubleChange>(this, starting_key_id,
                                               starting_key, new_value)
                    .release());
          });
  controls_form_pointer->addRow(tr("Starting &key:"),
                                starting_key_editor_pointer);

  starting_volume_percent_editor_pointer->setMinimum(1);
  starting_volume_percent_editor_pointer->setMaximum(MAX_STARTING_VOLUME);
  starting_volume_percent_editor_pointer->setDecimals(1);
  starting_volume_percent_editor_pointer->setSuffix("%");

  starting_volume_percent_editor_pointer->setValue(starting_volume_percent);

  connect(starting_volume_percent_editor_pointer, &QDoubleSpinBox::valueChanged,
          this, [this](double new_value) {
            Q_ASSERT(undo_stack_pointer != nullptr);
            undo_stack_pointer->push(std::make_unique<DoubleChange>(
                                         this, starting_volume_id,
                                         starting_volume_percent, new_value)
                                         .release());
          });
  controls_form_pointer->addRow(tr("Starting &volume percent:"),
                                starting_volume_percent_editor_pointer);

  starting_tempo_editor_pointer->setMinimum(MIN_STARTING_TEMPO);
  starting_tempo_editor_pointer->setValue(starting_tempo);
  starting_tempo_editor_pointer->setDecimals(1);
  starting_tempo_editor_pointer->setSuffix(" bpm");
  starting_tempo_editor_pointer->setMaximum(MAX_STARTING_TEMPO);

  connect(starting_tempo_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](double new_value) {
            Q_ASSERT(undo_stack_pointer != nullptr);
            undo_stack_pointer->push(
                std::make_unique<DoubleChange>(this, starting_tempo_id,
                                               starting_tempo, new_value)
                    .release());
          });
  controls_form_pointer->addRow(tr("Starting &tempo:"),
                                starting_tempo_editor_pointer);

  controls_pointer->setLayout(controls_form_pointer);

  dock_widget_pointer->setWidget(controls_pointer);
  dock_widget_pointer->setFeatures(QDockWidget::NoDockWidgetFeatures);
  addDockWidget(Qt::LeftDockWidgetArea, dock_widget_pointer);

  Q_ASSERT(chords_view_pointer != nullptr);
  connect(chords_view_pointer->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &SongEditor::update_actions);

  setWindowTitle("Justly");
  setCentralWidget(chords_view_pointer);

  connect(undo_stack_pointer, &QUndoStack::cleanChanged, this, [this]() {
    Q_ASSERT(save_action_pointer != nullptr);
    Q_ASSERT(undo_stack_pointer != nullptr);
    save_action_pointer->setEnabled(!undo_stack_pointer->isClean() &&
                                    !current_file.isEmpty());
  });

  Q_ASSERT(chords_view_pointer != nullptr);
  auto *chords_model_pointer = chords_view_pointer->chords_model_pointer;
  Q_ASSERT(chords_model_pointer != nullptr);
  connect(chords_model_pointer, &QAbstractItemModel::rowsInserted, this,
          &SongEditor::update_actions);
  connect(chords_model_pointer, &QAbstractItemModel::rowsInserted, this,
          &SongEditor::update_actions);
  connect(chords_model_pointer, &QAbstractItemModel::rowsRemoved, this,
          &SongEditor::update_actions);
  connect(chords_model_pointer, &QAbstractItemModel::modelReset, this,
          &SongEditor::update_actions);

  const auto *primary_screen_pointer = QGuiApplication::primaryScreen();
  Q_ASSERT(primary_screen_pointer != nullptr);
  resize(sizeHint().width(),
         primary_screen_pointer->availableGeometry().height());

  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->clear();
  undo_stack_pointer->setClean();

  fluid_event_set_dest(event_pointer, sequencer_id);

  start_real_time();
}

SongEditor::~SongEditor() {
  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->disconnect();

  delete_audio_driver();

  Q_ASSERT(event_pointer != nullptr);
  delete_fluid_event(event_pointer);

  Q_ASSERT(sequencer_pointer != nullptr);
  delete_fluid_sequencer(sequencer_pointer);

  Q_ASSERT(synth_pointer != nullptr);
  delete_fluid_synth(synth_pointer);

  Q_ASSERT(settings_pointer != nullptr);
  delete_fluid_settings(settings_pointer);
}

auto SongEditor::get_playback_volume() const -> float {
  Q_ASSERT(synth_pointer != nullptr);
  return fluid_synth_get_gain(synth_pointer);
}

void SongEditor::set_instrument_directly(const Instrument *new_value) {
  Q_ASSERT(starting_instrument_editor_pointer != nullptr);
  starting_instrument_editor_pointer->blockSignals(true);
  starting_instrument_editor_pointer->setValue(new_value);
  starting_instrument_editor_pointer->blockSignals(false);

  starting_instrument_pointer = new_value;
}

void SongEditor::set_double_directly(ChangeId change_id, double new_value) {
  if (change_id == starting_key_id) {
    Q_ASSERT(starting_key_editor_pointer != nullptr);
    starting_key_editor_pointer->blockSignals(true);
    starting_key_editor_pointer->setValue(new_value);
    starting_key_editor_pointer->blockSignals(false);

    starting_key = new_value;
  } else if (change_id == starting_volume_id) {
    Q_ASSERT(starting_volume_percent_editor_pointer != nullptr);
    starting_volume_percent_editor_pointer->blockSignals(true);
    starting_volume_percent_editor_pointer->setValue(new_value);
    starting_volume_percent_editor_pointer->blockSignals(false);

    starting_volume_percent = new_value;
  } else if (change_id == starting_tempo_id) {
    Q_ASSERT(starting_tempo_editor_pointer != nullptr);
    starting_tempo_editor_pointer->blockSignals(true);
    starting_tempo_editor_pointer->setValue(new_value);
    starting_tempo_editor_pointer->blockSignals(false);

    starting_tempo = new_value;
  } else {
    Q_ASSERT(false);
  }
}

void SongEditor::open_file(const QString &filename) {
  std::ifstream file_io(filename.toStdString().c_str());
  nlohmann::json json_song;
  try {
    json_song = nlohmann::json::parse(file_io);
  } catch (const nlohmann::json::parse_error &parse_error) {
    QMessageBox::warning(this, tr("Parsing error"), parse_error.what());
    return;
  }
  file_io.close();

  static const nlohmann::json_schema::json_validator song_validator =
      make_validator(
          "Song",
          nlohmann::json({{"description", "A Justly song in JSON format"},
                          {"type", "object"},
                          {"required",
                           {"starting_key", "starting_tempo",
                            "starting_volume_percent", "starting_instrument"}},
                          {"properties",
                           {{"starting_instrument",
                             {{"type", "string"},
                              {"description", "the starting instrument"},
                              {"enum", get_instrument_names()}}},
                            {"starting_key",
                             {{"type", "number"},
                              {"description", "the starting key, in Hz"},
                              {"minimum", MIN_STARTING_KEY},
                              {"maximum", MAX_STARTING_KEY}}},
                            {"starting_tempo",
                             {{"type", "number"},
                              {"description", "the starting tempo, in bpm"},
                              {"minimum", MIN_STARTING_TEMPO},
                              {"maximum", MAX_STARTING_TEMPO}}},
                            {"starting_volume_percent",
                             {{"type", "number"},
                              {"description",
                               "the starting volume percent, from 1 to 100"},
                              {"minimum", 1},
                              {"maximum", MAX_STARTING_VOLUME}}},
                            {"chords", get_chords_schema()}}}}));
  if (validate_json(this, json_song, song_validator)) {
    Q_ASSERT(json_song.contains("starting_key"));
    const auto &starting_key_value = json_song["starting_key"];
    Q_ASSERT(starting_key_value.is_number());
    Q_ASSERT(starting_key_editor_pointer != nullptr);
    starting_key_editor_pointer->setValue(starting_key_value.get<double>());

    Q_ASSERT(json_song.contains("starting_volume_percent"));
    const auto &starting_volume_value = json_song["starting_volume_percent"];
    Q_ASSERT(starting_volume_value.is_number());
    Q_ASSERT(starting_volume_percent_editor_pointer != nullptr);
    starting_volume_percent_editor_pointer->setValue(
        starting_volume_value.get<double>());

    Q_ASSERT(json_song.contains("starting_tempo"));
    const auto &starting_tempo_value = json_song["starting_tempo"];
    Q_ASSERT(starting_tempo_value.is_number());
    Q_ASSERT(starting_tempo_editor_pointer != nullptr);
    starting_tempo_editor_pointer->setValue(starting_tempo_value.get<double>());

    Q_ASSERT(json_song.contains("starting_instrument"));
    const auto &starting_instrument_value = json_song["starting_instrument"];
    Q_ASSERT(starting_instrument_value.is_string());
    Q_ASSERT(starting_instrument_editor_pointer != nullptr);
    starting_instrument_editor_pointer->setValue(
        get_instrument_pointer(starting_instrument_value.get<std::string>()));

    Q_ASSERT(chords_view_pointer != nullptr);
    auto *chords_model_pointer = chords_view_pointer->chords_model_pointer;
    Q_ASSERT(chords_model_pointer != nullptr);
    chords_model_pointer->delete_all_chords();

    if (json_song.contains("chords")) {
      chords_model_pointer->insert_json_chords(0, json_song["chords"]);
    }

    current_file = filename;

    Q_ASSERT(undo_stack_pointer != nullptr);
    undo_stack_pointer->clear();
    undo_stack_pointer->setClean();
  }
}

void SongEditor::save_as_file(const QString &filename) {
  std::ofstream file_io(filename.toStdString().c_str());

  nlohmann::json json_song;
  Q_ASSERT(starting_key_editor_pointer != nullptr);
  json_song["starting_key"] = starting_key;

  Q_ASSERT(starting_tempo_editor_pointer != nullptr);
  json_song["starting_tempo"] = starting_tempo;

  Q_ASSERT(starting_volume_percent_editor_pointer != nullptr);
  json_song["starting_volume_percent"] = starting_volume_percent;

  Q_ASSERT(starting_instrument_pointer != nullptr);
  json_song["starting_instrument"] =
      starting_instrument_pointer->instrument_name;

  Q_ASSERT(chords_view_pointer != nullptr);
  auto *chords_model_pointer = chords_view_pointer->chords_model_pointer;
  Q_ASSERT(chords_model_pointer != nullptr);
  json_song["chords"] = chords_model_pointer->copy_chords_to_json(
      0, chords_model_pointer->chords.size());

  file_io << std::setw(4) << json_song;
  file_io.close();
  current_file = filename;

  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->setClean();
}

void SongEditor::export_to_file(const QString &output_file) {
  stop_playing();

  delete_audio_driver();

  Q_ASSERT(settings_pointer != nullptr);
  fluid_settings_setstr(settings_pointer, "audio.driver", "file");
  fluid_settings_setstr(settings_pointer, "audio.file.name",
                        output_file.toStdString().c_str());
  fluid_settings_setint(settings_pointer, "synth.lock-memory", 0);

  Q_ASSERT(chords_view_pointer != nullptr);
  auto *chords_model_pointer = chords_view_pointer->chords_model_pointer;
  Q_ASSERT(chords_model_pointer != nullptr);

  initialize_play();
  auto final_time = play_chords(0, chords_model_pointer->chords.size(),
                                START_END_MILLISECONDS);

  Q_ASSERT(synth_pointer != nullptr);
#ifndef DISABLE_AUDIO
  audio_driver_pointer =
      new_fluid_audio_driver(settings_pointer, synth_pointer);
#endif

  auto time_step = (final_time - starting_time + START_END_MILLISECONDS) *
                   MILLISECONDS_PER_SECOND;
  QThread::usleep(to_unsigned(static_cast<int>(time_step)));
  stop_playing();

  start_real_time();
}
