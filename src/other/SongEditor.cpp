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
#include <QRect>
#include <QScreen>
#include <QSize>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QString>
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
#include <initializer_list>
#include <iomanip>
#include <memory>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <sstream> // IWYU pragma: keep
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
#include "other/json.hpp"

const auto MIN_STARTING_KEY = 60;
const auto DEFAULT_STARTING_KEY = 220;
const auto MAX_STARTING_KEY = 440;

const auto DEFAULT_STARTING_VOLUME = 50;
const auto MAX_STARTING_VOLUME = 100;

const auto MIN_STARTING_TEMPO = 25;
const auto DEFAULT_STARTING_TEMPO = 100;
const auto MAX_STARTING_TEMPO = 200;

const auto DEFAULT_STARTING_INSTRUMENT = "Marimba";

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
                            .filePath(SOUNDFONT_RELATIVE_PATH)
                            .toStdString();
  Q_ASSERT(std::filesystem::exists(soundfont_file));

  auto maybe_soundfont_id =
      fluid_synth_sfload(synth_pointer, soundfont_file.c_str(), 1);
  Q_ASSERT(maybe_soundfont_id != -1);
  return maybe_soundfont_id;
}

auto SongEditor::beat_time() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}

auto SongEditor::has_real_time() const -> bool {
  return audio_driver_pointer != nullptr;
}

void SongEditor::set_playback_volume(float new_value) const {
  Q_ASSERT(synth_pointer != nullptr);
  fluid_synth_set_gain(synth_pointer, new_value);
}

void SongEditor::start_real_time(const std::string &driver) {
  if (has_real_time()) {
    delete_fluid_audio_driver(audio_driver_pointer);
  }

  Q_ASSERT(settings_pointer != nullptr);
  fluid_settings_setint(settings_pointer, "synth.lock-memory", 1);
  fluid_settings_setstr(settings_pointer, "audio.driver", driver.c_str());

  Q_ASSERT(synth_pointer != nullptr);
#ifndef NO_SPEAKERS
  audio_driver_pointer =
      new_fluid_audio_driver(settings_pointer, synth_pointer);

  if (audio_driver_pointer == nullptr) {
    std::stringstream warning_message;
    warning_message << tr("Cannot start audio driver ").toStdString() << driver;
    QMessageBox::warning(this, tr("Audio driver error"),
                         warning_message.str().c_str());
  }
#endif
}

void SongEditor::initialize_play() {
  Q_ASSERT(starting_key_editor_pointer != nullptr);
  current_key = starting_key_editor_pointer->value();

  Q_ASSERT(starting_volume_editor_pointer != nullptr);
  current_volume = starting_volume_editor_pointer->value() / PERCENT;

  Q_ASSERT(starting_tempo_editor_pointer != nullptr);
  current_tempo = starting_tempo_editor_pointer->value();

  Q_ASSERT(current_instrument_pointer != nullptr);
  current_instrument_pointer = starting_instrument_editor_pointer->value();

  Q_ASSERT(sequencer_pointer != nullptr);
  starting_time = fluid_sequencer_get_tick(sequencer_pointer);
  current_time = starting_time;

  for (size_t index = 0; index < NUMBER_OF_MIDI_CHANNELS; index = index + 1) {
    Q_ASSERT(index < channel_schedules.size());
    channel_schedules[index] = static_cast<size_t>(current_time);
  }
}

void SongEditor::modulate(const Chord &chord) {
  current_key = current_key * chord.interval.ratio();
  current_volume = current_volume * chord.volume_ratio.ratio();
  current_tempo = current_tempo * chord.tempo_ratio.ratio();
  const auto &chord_instrument_pointer = chord.instrument_pointer;
  Q_ASSERT(chord_instrument_pointer != nullptr);
  if (!chord_instrument_pointer->instrument_name.empty()) {
    current_instrument_pointer = chord_instrument_pointer;
  }
}

auto SongEditor::play_notes(size_t chord_index, const Chord &chord,
                            size_t first_note_index, size_t number_of_notes)
    -> unsigned int {
  const auto &notes = chord.notes;
  unsigned int final_time = 0;
  auto notes_size = notes.size();
  for (auto note_index = first_note_index;
       note_index < first_note_index + number_of_notes;
       note_index = note_index + 1) {
    Q_ASSERT(note_index < notes_size);
    const auto &note = notes[note_index];

    const auto &note_instrument_pointer = note.instrument_pointer;

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
      std::stringstream warning_message;
      warning_message << "Out of MIDI channels for chord " << chord_index + 1
                      << ", note " << note_index + 1 << ". Not playing note.";
      QMessageBox::warning(this, tr("MIDI channel error"),
                           tr(warning_message.str().c_str()));
    } else {
      Q_ASSERT(current_time >= 0);
      auto int_current_time = static_cast<unsigned int>(current_time);

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
        std::stringstream warning_message;
        warning_message << "Volume exceeds 100% for chord " << chord_index + 1
                        << ", note " << note_index + 1
                        << ". Playing with 100% volume.";
        QMessageBox::warning(this, tr("Volume error"),
                             tr(warning_message.str().c_str()));
        new_volume = 1;
      }

      fluid_event_noteon(event_pointer, channel_number, int_closest_key,
                         static_cast<int16_t>(new_volume * MAX_VELOCITY));
      fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                              int_current_time + 2, 1);

      auto time_step =
          (beat_time() * note.beats.ratio() * note.tempo_ratio.ratio()) *
          MILLISECONDS_PER_SECOND;
      Q_ASSERT(time_step >= 0);
      const unsigned int end_time =
          int_current_time + static_cast<unsigned int>(time_step);

      fluid_event_noteoff(event_pointer, channel_number, int_closest_key);
      fluid_sequencer_send_at(sequencer_pointer, event_pointer, end_time, 1);

      Q_ASSERT(0 <= channel_number);
      Q_ASSERT(static_cast<size_t>(channel_number) < channel_schedules.size());
      channel_schedules[channel_number] = end_time;

      if (end_time > final_time) {
        final_time = end_time;
      }
    }
  }
  return final_time;
}

auto SongEditor::play_chords(size_t first_chord_index, size_t number_of_chords,
                             int wait_frames) -> unsigned int {
  Q_ASSERT(chords_view_pointer != nullptr);
  auto *chords_model_pointer = chords_view_pointer->chords_model_pointer;
  Q_ASSERT(chords_model_pointer != nullptr);
  const auto &chords = chords_model_pointer->chords;

  current_time = current_time + wait_frames;
  unsigned int final_time = 0;
  auto chords_size = chords.size();
  for (auto chord_index = first_chord_index;
       chord_index < first_chord_index + number_of_chords;
       chord_index = chord_index + 1) {
    Q_ASSERT(chord_index < chords_size);
    const auto &chord = chords[chord_index];

    modulate(chord);
    auto end_time = play_notes(chord_index, chord, 0, chord.notes.size());
    if (end_time > final_time) {
      final_time = end_time;
    }
    auto time_step =
        (beat_time() * chord.beats.ratio()) * MILLISECONDS_PER_SECOND;
    current_time = current_time + static_cast<unsigned int>(time_step);
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

void SongEditor::update_actions() const {
  Q_ASSERT(chords_view_pointer != nullptr);

  auto *chords_model_pointer = chords_view_pointer->model();
  Q_ASSERT(chords_model_pointer != nullptr);

  auto *selection_model = chords_view_pointer->selectionModel();
  Q_ASSERT(selection_model != nullptr);

  auto empty_container = chords_model_pointer->rowCount(QModelIndex()) == 0;

  auto selected_row_indexes = selection_model->selectedRows();

  auto any_rows_selected = !(selected_row_indexes.empty());

  auto cell_selected = false;

  if (any_rows_selected) {
    auto &first_index = selected_row_indexes[0];
    empty_container = get_level(first_index) == chord_level &&
                      selected_row_indexes.size() == 1 &&
                      chords_model_pointer->rowCount(first_index) == 0;
  } else {
    cell_selected = selection_model->selectedIndexes().size() == 1;
  }

  Q_ASSERT(copy_action_pointer != nullptr);
  copy_action_pointer->setEnabled(any_rows_selected || cell_selected);

  Q_ASSERT(insert_before_action_pointer != nullptr);
  insert_before_action_pointer->setEnabled(any_rows_selected);

  Q_ASSERT(insert_after_action_pointer != nullptr);
  insert_after_action_pointer->setEnabled(any_rows_selected);

  Q_ASSERT(insert_before_action_pointer != nullptr);
  insert_before_action_pointer->setEnabled(any_rows_selected);

  Q_ASSERT(remove_action_pointer != nullptr);
  remove_action_pointer->setEnabled(any_rows_selected);

  Q_ASSERT(play_action_pointer != nullptr);
  play_action_pointer->setEnabled(any_rows_selected);

  Q_ASSERT(paste_before_action_pointer != nullptr);
  paste_before_action_pointer->setEnabled(any_rows_selected);

  Q_ASSERT(paste_after_action_pointer != nullptr);
  paste_after_action_pointer->setEnabled(any_rows_selected);

  Q_ASSERT(insert_into_action_pointer != nullptr);
  insert_into_action_pointer->setEnabled(empty_container);

  Q_ASSERT(paste_into_action_pointer != nullptr);
  paste_into_action_pointer->setEnabled(empty_container);

  Q_ASSERT(paste_cell_action_pointer != nullptr);
  paste_cell_action_pointer->setEnabled(cell_selected);
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
      QFileDialog dialog(this, "Open — Justly", current_folder.c_str(),
                         "JSON file (*.json)");

      dialog.setAcceptMode(QFileDialog::AcceptOpen);
      dialog.setDefaultSuffix(".json");
      dialog.setFileMode(QFileDialog::ExistingFile);

      if (dialog.exec() != 0) {
        current_folder = dialog.directory().absolutePath().toStdString();
        const auto &selected_files = dialog.selectedFiles();
        Q_ASSERT(!(selected_files.empty()));
        open_file(selected_files[0].toStdString());
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
    QFileDialog dialog(this, "Save As — Justly", current_folder.c_str(),
                       "JSON file (*.json)");

    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix(".json");
    dialog.setFileMode(QFileDialog::AnyFile);

    if (dialog.exec() != 0) {
      current_folder = dialog.directory().absolutePath().toStdString();
      const auto &selected_files = dialog.selectedFiles();
      Q_ASSERT(!(selected_files.empty()));
      save_as_file(selected_files[0].toStdString());
    }
  });
  file_menu_pointer->addAction(save_as_action_pointer);
  save_as_action_pointer->setEnabled(true);

  auto *export_as_action_pointer =
      std::make_unique<QAction>(tr("&Export recording"), file_menu_pointer)
          .release();
  connect(export_as_action_pointer, &QAction::triggered, this, [this]() {
    QFileDialog dialog(this, "Export — Justly", current_folder.c_str(),
                       "WAV file (*.wav)");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix(".wav");
    dialog.setFileMode(QFileDialog::AnyFile);

    dialog.setLabelText(QFileDialog::Accept, "Export");

    if (dialog.exec() != 0) {
      current_folder = dialog.directory().absolutePath().toStdString();
      const auto &selected_files = dialog.selectedFiles();
      Q_ASSERT(!(selected_files.empty()));
      export_to_file(selected_files[0].toStdString());
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

  paste_cell_action_pointer->setEnabled(false);
  connect(paste_cell_action_pointer, &QAction::triggered, chords_view_pointer,
          &ChordsView::paste_cell);
  paste_cell_action_pointer->setShortcuts(QKeySequence::Paste);
  paste_menu_pointer->addAction(paste_cell_action_pointer);

  paste_before_action_pointer->setEnabled(false);
  connect(paste_before_action_pointer, &QAction::triggered, chords_view_pointer,
          &ChordsView::paste_before);
  paste_menu_pointer->addAction(paste_before_action_pointer);

  paste_after_action_pointer->setEnabled(false);
  connect(paste_after_action_pointer, &QAction::triggered, chords_view_pointer,
          &ChordsView::paste_after);
  paste_after_action_pointer->setShortcuts(QKeySequence::Paste);
  paste_menu_pointer->addAction(paste_after_action_pointer);

  paste_into_action_pointer->setEnabled(false);
  connect(paste_into_action_pointer, &QAction::triggered, chords_view_pointer,
          &ChordsView::paste_into);
  paste_menu_pointer->addAction(paste_into_action_pointer);

  edit_menu_pointer->addMenu(paste_menu_pointer);

  edit_menu_pointer->addSeparator();

  auto *insert_menu_pointer =
      std::make_unique<QMenu>(tr("&Insert"), edit_menu_pointer).release();

  edit_menu_pointer->addMenu(insert_menu_pointer);

  insert_before_action_pointer->setEnabled(false);
  connect(insert_before_action_pointer, &QAction::triggered,
          chords_view_pointer, &ChordsView::insert_before);
  insert_menu_pointer->addAction(insert_before_action_pointer);

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

  remove_action_pointer->setEnabled(false);
  remove_action_pointer->setShortcuts(QKeySequence::Delete);
  connect(remove_action_pointer, &QAction::triggered, chords_view_pointer,
          &ChordsView::remove_selected);
  edit_menu_pointer->addAction(remove_action_pointer);

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

    auto parent_number =
        to_parent_number(chords_model_pointer->parent(first_index));
    auto first_child_number = first_index.row();
    auto number_of_children = selected_row_indexes.size();

    stop_playing();
    initialize_play();
    const auto &chords = chords_model_pointer->chords;
    auto chords_size = chords.size();
    if (parent_number == -1) {
      for (size_t chord_index = 0;
           chord_index < static_cast<size_t>(first_child_number);
           chord_index = chord_index + 1) {
        Q_ASSERT(static_cast<size_t>(chord_index) < chords_size);
        modulate(chords[chord_index]);
      }
      play_chords(first_child_number, number_of_children);
    } else {
      Q_ASSERT(parent_number >= 0);
      auto unsigned_parent_number = static_cast<size_t>(parent_number);
      for (size_t chord_index = 0; chord_index < unsigned_parent_number;
           chord_index = chord_index + 1) {
        Q_ASSERT(chord_index < chords_size);
        modulate(chords[chord_index]);
      }

      Q_ASSERT(unsigned_parent_number < chords_size);
      const auto &chord = chords[unsigned_parent_number];
      modulate(chord);
      play_notes(parent_number, chord, first_child_number, number_of_children);
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

  starting_instrument_editor_pointer->setValue(
      get_instrument_pointer(DEFAULT_STARTING_INSTRUMENT));
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

  starting_key_editor_pointer->setValue(DEFAULT_STARTING_KEY);
  starting_key = DEFAULT_STARTING_KEY;

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

  starting_volume_editor_pointer->setMinimum(1);
  starting_volume_editor_pointer->setMaximum(MAX_STARTING_VOLUME);
  starting_volume_editor_pointer->setDecimals(1);
  starting_volume_editor_pointer->setSuffix("%");

  starting_volume_editor_pointer->setValue(DEFAULT_STARTING_VOLUME);
  starting_volume_percent = DEFAULT_STARTING_VOLUME;

  connect(starting_volume_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](double new_value) {
            Q_ASSERT(undo_stack_pointer != nullptr);
            undo_stack_pointer->push(std::make_unique<DoubleChange>(
                                         this, starting_volume_id,
                                         starting_volume_percent, new_value)
                                         .release());
          });
  controls_form_pointer->addRow(tr("Starting &volume:"),
                                starting_volume_editor_pointer);

  starting_tempo_editor_pointer->setMinimum(MIN_STARTING_TEMPO);
  starting_tempo_editor_pointer->setValue(DEFAULT_STARTING_TEMPO);
  starting_tempo_editor_pointer->setDecimals(1);
  starting_tempo_editor_pointer->setSuffix(" bpm");

  starting_tempo = DEFAULT_STARTING_TEMPO;
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
                                    !current_file.empty());
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

  if (has_real_time()) {
    delete_fluid_audio_driver(audio_driver_pointer);
  }

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
    Q_ASSERT(starting_volume_editor_pointer != nullptr);
    starting_volume_editor_pointer->blockSignals(true);
    starting_volume_editor_pointer->setValue(new_value);
    starting_volume_editor_pointer->blockSignals(false);

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

void SongEditor::open_file(const std::string &filename) {
  std::ifstream file_io(filename.c_str());
  nlohmann::json json_song;
  try {
    json_song = nlohmann::json::parse(file_io);
  } catch (const nlohmann::json::parse_error &parse_error) {
    QMessageBox::warning(this, tr("Parsing error"), parse_error.what());
    return;
  }

  file_io.close();
  static const nlohmann::json_schema::json_validator validator(
      nlohmann::json({{"$schema", "http://json-schema.org/draft-07/schema#"},
                      {"title", "Song"},
                      {"description", "A Justly song in JSON format"},
                      {"type", "object"},
                      {"required",
                       {"starting_key", "starting_tempo", "starting_volume",
                        "starting_instrument"}},
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
                        {"starting_volume",
                         {{"type", "number"},
                          {"description", "the starting volume, from 1 to 100"},
                          {"minimum", 1},
                          {"maximum", MAX_STARTING_VOLUME}}},
                        {"chords",
                         {{"type", "array"},
                          {"description", "a list of chords"},
                          {"items", get_chord_schema()}}}}}}));
  if (validate(this, json_song, validator)) {
    Q_ASSERT(json_song.contains("starting_key"));
    const auto &starting_key_value = json_song["starting_key"];
    Q_ASSERT(starting_key_value.is_number());
    starting_key_editor_pointer->setValue(starting_key_value.get<double>());

    Q_ASSERT(json_song.contains("starting_volume"));
    const auto &starting_volume_value = json_song["starting_volume"];
    Q_ASSERT(starting_volume_value.is_number());
    starting_volume_editor_pointer->setValue(
        starting_volume_value.get<double>());

    Q_ASSERT(json_song.contains("starting_tempo"));
    const auto &starting_tempo_value = json_song["starting_tempo"];
    Q_ASSERT(starting_tempo_value.is_number());
    starting_tempo_editor_pointer->setValue(starting_tempo_value.get<double>());

    Q_ASSERT(json_song.contains("starting_instrument"));
    const auto &starting_instrument_value = json_song["starting_instrument"];
    Q_ASSERT(starting_instrument_value.is_string());
    starting_instrument_editor_pointer->setValue(
        get_instrument_pointer(starting_instrument_value.get<std::string>()));

    Q_ASSERT(chords_view_pointer != nullptr);
    auto *chords_model_pointer = chords_view_pointer->chords_model_pointer;
    Q_ASSERT(chords_model_pointer != nullptr);
    chords_model_pointer->load_chords(json_song);

    current_file = filename;

    Q_ASSERT(undo_stack_pointer != nullptr);
    undo_stack_pointer->clear();
    undo_stack_pointer->setClean();
  }
}

void SongEditor::save_as_file(const std::string &filename) {
  std::ofstream file_io(filename.c_str());

  nlohmann::json json_song;
  Q_ASSERT(starting_key_editor_pointer != nullptr);
  json_song["starting_key"] = starting_key_editor_pointer->value();

  Q_ASSERT(starting_tempo_editor_pointer != nullptr);
  json_song["starting_tempo"] = starting_tempo_editor_pointer->value();

  Q_ASSERT(starting_volume_editor_pointer != nullptr);
  json_song["starting_volume"] = starting_volume_editor_pointer->value();

  Q_ASSERT(starting_instrument_editor_pointer != nullptr);
  const auto *starting_instrument_pointer =
      starting_instrument_editor_pointer->value();

  Q_ASSERT(starting_instrument_pointer != nullptr);
  json_song["starting_instrument"] =
      starting_instrument_pointer->instrument_name;

  Q_ASSERT(chords_view_pointer != nullptr);
  auto *chords_model_pointer = chords_view_pointer->chords_model_pointer;
  Q_ASSERT(chords_model_pointer != nullptr);
  const auto &chords = chords_model_pointer->chords;
  json_song["chords"] = objects_to_json(chords, 0, chords.size());

  file_io << std::setw(4) << json_song;
  file_io.close();
  current_file = filename;

  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->setClean();
}

void SongEditor::export_to_file(const std::string &output_file) {
  stop_playing();

  if (has_real_time()) {
    delete_fluid_audio_driver(audio_driver_pointer);
  }

  Q_ASSERT(settings_pointer != nullptr);
  fluid_settings_setstr(settings_pointer, "audio.driver", "file");
  fluid_settings_setstr(settings_pointer, "audio.file.name",
                        output_file.c_str());
  fluid_settings_setint(settings_pointer, "synth.lock-memory", 0);

  Q_ASSERT(chords_view_pointer != nullptr);
  auto *chords_model_pointer = chords_view_pointer->chords_model_pointer;
  Q_ASSERT(chords_model_pointer != nullptr);

  initialize_play();
  auto final_time = play_chords(0, chords_model_pointer->chords.size(),
                                START_END_MILLISECONDS);

  Q_ASSERT(synth_pointer != nullptr);
#ifndef NO_SPEAKERS
  audio_driver_pointer =
      new_fluid_audio_driver(settings_pointer, synth_pointer);
#endif

  auto time_step = (final_time - starting_time + START_END_MILLISECONDS) *
                   MILLISECONDS_PER_SECOND;
  Q_ASSERT(time_step >= 0);
  QThread::usleep(static_cast<uint64_t>(time_step));
  stop_playing();

  start_real_time();
}
