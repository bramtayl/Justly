#include "justly/SongEditor.hpp"

#include <QAbstractItemModel>
#include <QAction>
#include <QCloseEvent>
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
#include <QSpinBox>
#include <QStandardPaths>
#include <QString>
#include <QTextStream>
#include <QUndoStack>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <vector>

#include "cell_editors/InstrumentEditor.hpp"
#include "cell_values/instruments.hpp"
#include "changes/SetGain.hpp"
#include "changes/SetStartingInstrument.hpp"
#include "changes/SetStartingKey.hpp"
#include "changes/SetStartingTempo.hpp"
#include "changes/SetStartingVelocity.hpp"
#include "indices/index_functions.hpp"
#include "justly/Instrument.hpp"
#include "justly/Interval.hpp"
#include "justly/Rational.hpp"
#include "models/ChordsModel.hpp"
#include "note_chord/Chord.hpp"
#include "note_chord/Note.hpp"
#include "other/ChordsView.hpp"
#include "other/conversions.hpp"
#include "other/schemas.hpp"

const auto NUMBER_OF_MIDI_CHANNELS = 16;

const auto DEFAULT_STARTING_KEY = 220;
const auto DEFAULT_STARTING_TEMPO = 100;
const auto DEFAULT_STARTING_VELOCITY = 64;

const auto MAX_GAIN = 10;
const auto DEFAULT_GAIN = 1;
const auto GAIN_STEP = 0.1;

const auto MIN_STARTING_KEY = 60;
const auto MAX_STARTING_KEY = 440;

const auto MIN_STARTING_TEMPO = 25;
const auto MAX_STARTING_TEMPO = 200;

const auto HALFSTEPS_PER_OCTAVE = 12;
const auto CONCERT_A_FREQUENCY = 440;
const auto CONCERT_A_MIDI = 69;

const auto SECONDS_PER_MINUTE = 60;
const unsigned int MILLISECONDS_PER_SECOND = 1000;

const auto MAX_VELOCITY = 127;

const auto BEND_PER_HALFSTEP = 4096;
const auto ZERO_BEND_HALFSTEPS = 2;

// insert end buffer at the end of songs
const unsigned int START_END_MILLISECONDS = 500;

const auto VERBOSE_FLUIDSYNTH = false;

const auto OCTAVE_RATIO = 2.0;

void set_value_no_signals(QDoubleSpinBox *spinbox_pointer, double new_value) {
  Q_ASSERT(spinbox_pointer != nullptr);
  spinbox_pointer->blockSignals(true);
  spinbox_pointer->setValue(new_value);
  spinbox_pointer->blockSignals(false);
}

auto interval_to_double(const Interval &interval) -> double {
  Q_ASSERT(interval.denominator != 0);
  return (1.0 * interval.numerator) / interval.denominator *
         pow(OCTAVE_RATIO, interval.octave);
}

auto rational_to_double(const Rational &rational) -> double {
  Q_ASSERT(rational.denominator != 0);
  return (1.0 * rational.numerator) / rational.denominator;
}

[[nodiscard]] auto get_settings_pointer() -> fluid_settings_t * {
  fluid_settings_t *settings_pointer = new_fluid_settings();
  Q_ASSERT(settings_pointer != nullptr);
  auto cores = std::thread::hardware_concurrency();
  if (cores > 0) {
    auto cores_result = fluid_settings_setint(
        settings_pointer, "synth.cpu-cores", static_cast<int>(cores));
    Q_ASSERT(cores_result == FLUID_OK);
  }
  if (VERBOSE_FLUIDSYNTH) {
    auto verbose_result =
        fluid_settings_setint(settings_pointer, "synth.verbose", 1);
    Q_ASSERT(verbose_result == FLUID_OK);
  }
  return settings_pointer;
}

[[nodiscard]] auto get_soundfont_id(fluid_synth_t *synth_pointer) -> int {
  auto soundfont_file = QDir(QCoreApplication::applicationDirPath())
                            .filePath("../share/MuseScore_General.sf2")
                            .toStdString();
  Q_ASSERT(std::filesystem::exists(soundfont_file));

  auto maybe_soundfont_id =
      fluid_synth_sfload(synth_pointer, soundfont_file.c_str(), 1);
  Q_ASSERT(maybe_soundfont_id != -1);
  return maybe_soundfont_id;
}

[[nodiscard]] auto get_json_value(const nlohmann::json &json,
                                  const std::string &field) {
  Q_ASSERT(json.contains(field));
  return json[field];
}

[[nodiscard]] auto get_json_double(const nlohmann::json &json,
                                   const std::string &field) {
  const auto &json_value = get_json_value(json, field);
  Q_ASSERT(json_value.is_number());
  return json_value.get<double>();
}

void register_converters() {
  QMetaType::registerConverter<Rational, QString>([](const Rational &rational) {
    auto numerator = rational.numerator;
    auto denominator = rational.denominator;

    QString result;
    QTextStream stream(&result);
    if (numerator != 1) {
      stream << numerator;
    }
    if (denominator != 1) {
      stream << "/" << denominator;
    }
    return result;
  });
  QMetaType::registerConverter<Interval, QString>([](const Interval &interval) {
    auto numerator = interval.numerator;
    auto denominator = interval.denominator;
    auto octave = interval.octave;

    QString result;
    QTextStream stream(&result);
    if (numerator != 1) {
      stream << numerator;
    }
    if (denominator != 1) {
      stream << "/" << denominator;
    }
    if (octave != 0) {
      stream << "o" << octave;
    }
    return result;
  });
  QMetaType::registerConverter<const Instrument *, QString>(
      [](const Instrument *instrument_pointer) {
        Q_ASSERT(instrument_pointer != nullptr);
        return QString::fromStdString(instrument_pointer->instrument_name);
      });
}

auto SongEditor::ask_discard_changes() -> bool {
  return QMessageBox::question(this, tr("Unsaved changes"),
                               tr("Discard unsaved changes?")) ==
         QMessageBox::Yes;
}

auto SongEditor::get_selected_file(QFileDialog *dialog_pointer) -> QString {
  current_folder = dialog_pointer->directory().absolutePath();
  const auto &selected_files = dialog_pointer->selectedFiles();
  Q_ASSERT(!(selected_files.empty()));
  return selected_files[0];
}

auto SongEditor::beat_time() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}

void SongEditor::send_event_at(double time) const {
  Q_ASSERT(sequencer_pointer != nullptr);
  Q_ASSERT(event_pointer != nullptr);
  Q_ASSERT(time >= 0);
  auto result =
      fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                              static_cast<unsigned int>(std::round(time)), 1);
  Q_ASSERT(result == FLUID_OK);
};

void SongEditor::start_real_time() {
  static const std::string default_driver = [this]() -> std::string {
    auto default_driver_pointer = std::make_unique<char *>();
    fluid_settings_dupstr(settings_pointer, "audio.driver",
                          default_driver_pointer.get());
    Q_ASSERT(default_driver_pointer != nullptr);
    return *default_driver_pointer;
  }();

  delete_audio_driver();

  Q_ASSERT(settings_pointer != nullptr);

#ifdef __linux__
  fluid_settings_setstr(settings_pointer, "audio.driver", "pulseaudio");
#else
  fluid_settings_setstr(settings_pointer, "audio.driver",
                        default_driver.c_str());
#endif

#ifndef NO_REALTIME_AUDIO
  Q_ASSERT(synth_pointer != nullptr);
  audio_driver_pointer =
      new_fluid_audio_driver(settings_pointer, synth_pointer);
#endif
  if (audio_driver_pointer == nullptr) {
    QString message;
    QTextStream stream(&message);
    stream << tr("Cannot start audio driver \"")
           << QString::fromStdString(default_driver) << tr("\"");
#ifdef NO_WARN_AUDIO
    qWarning("%s", message.toStdString().c_str());
#else
    QMessageBox::warning(this, tr("Audio driver error"), message);
#endif
  }
}

void SongEditor::initialize_play() {
  Q_ASSERT(starting_key_editor_pointer != nullptr);
  current_key = starting_key_editor_pointer->value();

  Q_ASSERT(starting_velocity_editor_pointer != nullptr);
  current_velocity = starting_velocity_editor_pointer->value();

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
  current_key = current_key * interval_to_double(chord.interval);
  current_velocity =
      current_velocity * rational_to_double(chord.velocity_ratio);
  current_tempo = current_tempo * rational_to_double(chord.tempo_ratio);
  const auto *chord_instrument_pointer = chord.instrument_pointer;
  Q_ASSERT(chord_instrument_pointer != nullptr);
  if (!instrument_is_default(*chord_instrument_pointer)) {
    current_instrument_pointer = chord_instrument_pointer;
  }
}

auto SongEditor::play_notes(size_t chord_index, const Chord &chord,
                            size_t first_note_index,
                            size_t number_of_notes) -> double {
  auto final_time = current_time;
  for (auto note_index = first_note_index;
       note_index < first_note_index + number_of_notes;
       note_index = note_index + 1) {
    const auto &note = chord.get_const_note(note_index);

    const auto *note_instrument_pointer = note.instrument_pointer;

    Q_ASSERT(note_instrument_pointer != nullptr);
    const auto &instrument_pointer =
        (instrument_is_default(*note_instrument_pointer)
             ? current_instrument_pointer
             : note_instrument_pointer);

    Q_ASSERT(CONCERT_A_FREQUENCY != 0);
    auto key_float = HALFSTEPS_PER_OCTAVE *
                         log2(current_key * interval_to_double(note.interval) /
                              CONCERT_A_FREQUENCY) +
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
      Q_ASSERT(instrument_pointer != nullptr);
      Q_ASSERT(event_pointer != nullptr);
      fluid_event_program_select(event_pointer, channel_number, soundfont_id,
                                 instrument_pointer->bank_number,
                                 instrument_pointer->preset_number);
      send_event_at(current_time);

      fluid_event_pitch_bend(
          event_pointer, channel_number,
          static_cast<int>(
              std::round((key_float - closest_key + ZERO_BEND_HALFSTEPS) *
                         BEND_PER_HALFSTEP)));
      send_event_at(current_time + 1);

      auto new_velocity =
          current_velocity * rational_to_double(note.velocity_ratio);
      if (new_velocity > MAX_VELOCITY) {
        QString message;
        QTextStream stream(&message);
        stream << tr("Velocity exceeds ") << MAX_VELOCITY << tr(" for chord ")
               << chord_index + 1 << tr(", note ") << note_index + 1
               << tr(". Playing with velocity ") << MAX_VELOCITY << tr(".");
        QMessageBox::warning(this, tr("Velocity error"), message);
        new_velocity = 1;
      }

      fluid_event_noteon(event_pointer, channel_number, int_closest_key,
                         static_cast<int16_t>(std::round(new_velocity)));
      send_event_at(current_time + 2);

      auto end_time =
          current_time + (beat_time() * rational_to_double(note.beats) *
                          rational_to_double(note.tempo_ratio)) *
                             MILLISECONDS_PER_SECOND;

      fluid_event_noteoff(event_pointer, channel_number, int_closest_key);
      send_event_at(end_time);
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
  auto final_time = current_time;
  for (auto chord_index = first_chord_number;
       chord_index < first_chord_number + number_of_chords;
       chord_index = chord_index + 1) {
    const auto &chord = chords_model_pointer->get_const_chord(chord_index);

    modulate(chord);
    auto end_time =
        play_notes(chord_index, chord, 0, chord.get_number_of_notes());
    if (end_time > final_time) {
      final_time = end_time;
    }
    current_time =
        current_time + (beat_time() * rational_to_double(chord.beats)) *
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

void SongEditor::delete_audio_driver() {
  if (audio_driver_pointer != nullptr) {
    delete_fluid_audio_driver(audio_driver_pointer);
    audio_driver_pointer = nullptr;
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
  auto chords_selected =
      any_rows_selected && valid_is_chord_index(selected_row_indexes[0]);
  auto can_contain = chords_model_pointer->rowCount(QModelIndex()) == 0 ||
                     (chords_selected && selected_row_indexes.size() == 1);

  Q_ASSERT(cut_action_pointer != nullptr);
  cut_action_pointer->setEnabled(anything_selected);

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

  Q_ASSERT(expand_action_pointer != nullptr);
  expand_action_pointer->setEnabled(chords_selected);

  Q_ASSERT(collapse_action_pointer != nullptr);
  collapse_action_pointer->setEnabled(chords_selected);
}

SongEditor::SongEditor(QWidget *parent_pointer, Qt::WindowFlags flags)
    : QMainWindow(parent_pointer, flags), gain(DEFAULT_GAIN),
      starting_instrument_pointer(get_instrument_pointer("Cello")),
      starting_key(DEFAULT_STARTING_KEY),
      starting_velocity(DEFAULT_STARTING_VELOCITY),
      starting_tempo(DEFAULT_STARTING_TEMPO),
      gain_editor_pointer(new QDoubleSpinBox(this)),
      starting_instrument_editor_pointer(new InstrumentEditor(this, false)),
      starting_key_editor_pointer(new QDoubleSpinBox(this)),
      starting_velocity_editor_pointer(new QDoubleSpinBox(this)),
      starting_tempo_editor_pointer(new QDoubleSpinBox(this)),
      undo_stack_pointer(new QUndoStack(this)),
      chords_view_pointer(new ChordsView(undo_stack_pointer, this)),
      insert_after_action_pointer(new QAction(tr("Row &after"), this)),
      insert_into_action_pointer(new QAction(tr("Row &into start"), this)),
      delete_action_pointer(new QAction(tr("&Delete"), this)),
      cut_action_pointer(new QAction(tr("&Cut"), this)),
      copy_action_pointer(new QAction(tr("&Copy"), this)),
      paste_cells_or_after_action_pointer(
          new QAction(tr("&Cells, or Rows &after"), this)),
      paste_into_action_pointer(new QAction(tr("Rows &into start"), this)),
      save_action_pointer(new QAction(tr("&Save"), this)),
      play_action_pointer(new QAction(tr("&Play selection"), this)),
      stop_playing_action_pointer(new QAction(tr("&Stop playing"), this)),
      expand_action_pointer(new QAction(tr("&Expand"), this)),
      collapse_action_pointer(new QAction(tr("&Collapse"), this)),
      current_folder(
          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)),
      current_instrument_pointer(get_instrument_pointer("")),
      channel_schedules(std::vector<double>(NUMBER_OF_MIDI_CHANNELS, 0)),
      settings_pointer(get_settings_pointer()),
      event_pointer(new_fluid_event()),
      sequencer_pointer(new_fluid_sequencer2(0)),
      synth_pointer(new_fluid_synth(settings_pointer)),
      soundfont_id(get_soundfont_id(synth_pointer)),
      sequencer_id(fluid_sequencer_register_fluidsynth(sequencer_pointer,
                                                       synth_pointer)) {
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
    if (undo_stack_pointer->isClean() || ask_discard_changes()) {
      auto *dialog_pointer = make_file_dialog(
          tr("Open — Justly"), "JSON file (*.json)", QFileDialog::AcceptOpen,
          ".json", QFileDialog::ExistingFile);
      if (dialog_pointer->exec() != 0) {
        open_file(get_selected_file(dialog_pointer));
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
    auto *dialog_pointer = make_file_dialog(
        tr("Save As — Justly"), "JSON file (*.json)", QFileDialog::AcceptSave,
        ".json", QFileDialog::AnyFile);

    if (dialog_pointer->exec() != 0) {
      save_as_file(get_selected_file(dialog_pointer));
    }
  });
  file_menu_pointer->addAction(save_as_action_pointer);
  save_as_action_pointer->setEnabled(true);

  auto *export_action_pointer =
      std::make_unique<QAction>(tr("&Export recording"), file_menu_pointer)
          .release();
  connect(export_action_pointer, &QAction::triggered, this, [this]() {
    auto *dialog_pointer =
        make_file_dialog(tr("Export — Justly"), "WAV file (*.wav)",
                         QFileDialog::AcceptSave, ".wav", QFileDialog::AnyFile);
    dialog_pointer->setLabelText(QFileDialog::Accept, "Export");
    if (dialog_pointer->exec() != 0) {
      export_to_file(get_selected_file(dialog_pointer));
    }
  });
  file_menu_pointer->addAction(export_action_pointer);

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

  cut_action_pointer->setEnabled(false);
  cut_action_pointer->setShortcuts(QKeySequence::Cut);
  connect(cut_action_pointer, &QAction::triggered, chords_view_pointer,
          &ChordsView::cut_selected);
  edit_menu_pointer->addAction(cut_action_pointer);

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

  expand_action_pointer->setEnabled(false);
  connect(expand_action_pointer, &QAction::triggered, chords_view_pointer,
          &ChordsView::expand_selected);
  view_menu_pointer->addAction(expand_action_pointer);

  collapse_action_pointer->setEnabled(false);
  connect(collapse_action_pointer, &QAction::triggered, chords_view_pointer,
          &ChordsView::collapse_selected);
  view_menu_pointer->addAction(collapse_action_pointer);

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

    auto first_child_number = get_child_number(first_index);
    auto number_of_children = selected_row_indexes.size();

    stop_playing();
    initialize_play();
    if (is_root_index(parent_index)) {
      for (size_t chord_index = 0; chord_index < first_child_number;
           chord_index = chord_index + 1) {
        modulate(chords_model_pointer->get_const_chord(chord_index));
      }
      play_chords(first_child_number, number_of_children);
    } else {
      auto chord_number = get_child_number(parent_index);
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

  gain_editor_pointer->setMinimum(0);
  gain_editor_pointer->setMaximum(MAX_GAIN);
  gain_editor_pointer->setSingleStep(GAIN_STEP);
  connect(gain_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](double new_value) {
            Q_ASSERT(undo_stack_pointer != nullptr);
            undo_stack_pointer->push(
                std::make_unique<SetGain>(this, gain, new_value).release());
          });
  set_gain_directly(DEFAULT_GAIN);
  controls_form_pointer->addRow(tr("&Gain:"), gain_editor_pointer);

  starting_instrument_editor_pointer->setValue(starting_instrument_pointer);
  starting_instrument_pointer = starting_instrument_editor_pointer->value();
  connect(starting_instrument_editor_pointer, &QComboBox::currentIndexChanged,
          this, [this](int new_index) {
            Q_ASSERT(undo_stack_pointer != nullptr);
            undo_stack_pointer->push(std::make_unique<SetStartingInstrument>(
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
                std::make_unique<SetStartingKey>(this, starting_key, new_value)
                    .release());
          });
  controls_form_pointer->addRow(tr("Starting &key:"),
                                starting_key_editor_pointer);

  starting_velocity_editor_pointer->setMinimum(0);
  starting_velocity_editor_pointer->setMaximum(MAX_VELOCITY);
  starting_velocity_editor_pointer->setDecimals(1);
  starting_velocity_editor_pointer->setValue(starting_velocity);
  connect(starting_velocity_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](double new_value) {
            Q_ASSERT(undo_stack_pointer != nullptr);
            undo_stack_pointer->push(std::make_unique<SetStartingVelocity>(
                                         this, starting_velocity, new_value)
                                         .release());
          });
  controls_form_pointer->addRow(tr("Starting &velocity:"),
                                starting_velocity_editor_pointer);

  starting_tempo_editor_pointer->setMinimum(MIN_STARTING_TEMPO);
  starting_tempo_editor_pointer->setValue(starting_tempo);
  starting_tempo_editor_pointer->setDecimals(1);
  starting_tempo_editor_pointer->setSuffix(" bpm");
  starting_tempo_editor_pointer->setMaximum(MAX_STARTING_TEMPO);

  connect(starting_tempo_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](double new_value) {
            Q_ASSERT(undo_stack_pointer != nullptr);
            undo_stack_pointer->push(std::make_unique<SetStartingTempo>(
                                         this, starting_tempo, new_value)
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

void SongEditor::closeEvent(QCloseEvent *event_pointer) {
  Q_ASSERT(undo_stack_pointer != nullptr);
  Q_ASSERT(event_pointer != nullptr);
  if (!undo_stack_pointer->isClean() && !ask_discard_changes()) {
    event_pointer->ignore();
    return;
  }
  QMainWindow::closeEvent(event_pointer);
}

auto SongEditor::get_chords_view_pointer() const -> QTreeView * {
  return chords_view_pointer;
}

auto SongEditor::get_gain() const -> double {
  Q_ASSERT(synth_pointer != nullptr);
  return fluid_synth_get_gain(synth_pointer);
};

auto SongEditor::get_starting_instrument() const -> const Instrument * {
  return starting_instrument_pointer;
};

auto SongEditor::get_starting_key() const -> double { return starting_key; };

auto SongEditor::get_starting_velocity() const -> double {
  return starting_velocity;
};

auto SongEditor::get_starting_tempo() const -> double {
  return starting_tempo;
};

auto SongEditor::get_current_file() const -> QString { return current_file; };

auto SongEditor::get_chord_index(size_t chord_number,
                                 NoteChordColumn note_chord_column) const
    -> QModelIndex {
  return chords_view_pointer->get_chord_index(chord_number, note_chord_column);
}

auto SongEditor::get_note_index(size_t chord_number, size_t note_number,
                                NoteChordColumn note_chord_column) const
    -> QModelIndex {
  return chords_view_pointer->get_note_index(chord_number, note_number,
                                             note_chord_column);
}

void SongEditor::set_gain(double new_value) const {
  gain_editor_pointer->setValue(new_value);
};

void SongEditor::set_starting_instrument(const Instrument *new_value) const {
  starting_instrument_editor_pointer->setValue(new_value);
}

void SongEditor::set_starting_key(double new_value) const {
  starting_key_editor_pointer->setValue(new_value);
}

void SongEditor::set_starting_velocity(double new_value) const {
  starting_velocity_editor_pointer->setValue(new_value);
}

void SongEditor::set_starting_tempo(double new_value) const {
  starting_tempo_editor_pointer->setValue(new_value);
}

void SongEditor::set_gain_directly(double new_gain) {
  Q_ASSERT(starting_instrument_editor_pointer != nullptr);
  gain_editor_pointer->blockSignals(true);
  gain_editor_pointer->setValue(new_gain);
  gain_editor_pointer->blockSignals(false);

  gain = new_gain;
  Q_ASSERT(synth_pointer != nullptr);
  fluid_synth_set_gain(synth_pointer, static_cast<float>(gain));
}

void SongEditor::set_starting_instrument_directly(const Instrument *new_value) {
  Q_ASSERT(starting_instrument_editor_pointer != nullptr);
  starting_instrument_editor_pointer->blockSignals(true);
  starting_instrument_editor_pointer->setValue(new_value);
  starting_instrument_editor_pointer->blockSignals(false);

  starting_instrument_pointer = new_value;
}

void SongEditor::set_starting_key_directly(double new_value) {
  set_value_no_signals(starting_key_editor_pointer, new_value);
  starting_key = new_value;
}

void SongEditor::set_starting_velocity_directly(double new_value) {
  set_value_no_signals(starting_velocity_editor_pointer, new_value);
  starting_velocity = new_value;
}

auto SongEditor::create_editor(QModelIndex index) const -> QWidget * {
  return chords_view_pointer->create_editor(index);
}

void SongEditor::set_editor(QWidget *cell_editor_pointer, QModelIndex index,
                            const QVariant &new_value) const {
  chords_view_pointer->set_editor(cell_editor_pointer, index, new_value);
}

void SongEditor::undo() const { undo_stack_pointer->undo(); };
void SongEditor::redo() const { undo_stack_pointer->redo(); };

void SongEditor::trigger_insert_after() const {
  insert_after_action_pointer->trigger();
};
void SongEditor::trigger_insert_into() const {
  insert_into_action_pointer->trigger();
};
void SongEditor::trigger_delete() const { delete_action_pointer->trigger(); };

void SongEditor::trigger_cut() const { cut_action_pointer->trigger(); };
void SongEditor::trigger_copy() const { copy_action_pointer->trigger(); };
void SongEditor::trigger_paste_cells_or_after() const {
  paste_cells_or_after_action_pointer->trigger();
};
void SongEditor::trigger_paste_into() const {
  paste_into_action_pointer->trigger();
};

void SongEditor::trigger_save() const { save_action_pointer->trigger(); };

void SongEditor::trigger_play() const { play_action_pointer->trigger(); };

void SongEditor::trigger_stop_playing() const {
  stop_playing_action_pointer->trigger();
};

void SongEditor::trigger_expand() const { expand_action_pointer->trigger(); };

void SongEditor::trigger_collapse() const {
  collapse_action_pointer->trigger();
};

auto SongEditor::make_file_dialog(
    const QString &caption, const QString &filter,
    QFileDialog::AcceptMode accept_mode, const QString &suffix,
    QFileDialog::FileMode file_mode) -> QFileDialog * {
  auto *dialog_pointer =
      std::make_unique<QFileDialog>(this, caption, current_folder, filter)
          .release();

  dialog_pointer->setAcceptMode(accept_mode);
  dialog_pointer->setDefaultSuffix(suffix);
  dialog_pointer->setFileMode(file_mode);

  return dialog_pointer;
}

void SongEditor::set_starting_tempo_directly(double new_value) {
  set_value_no_signals(starting_tempo_editor_pointer, new_value);
  starting_tempo = new_value;
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
          nlohmann::json(
              {{"description", "A Justly song in JSON format"},
               {"type", "object"},
               {"required",
                {"starting_key", "starting_tempo", "starting_velocity",
                 "starting_instrument"}},
               {"properties",
                {{"starting_instrument",
                  get_instrument_schema("the starting instrument")},
                 {"gain",
                  {{"type", "number"},
                   {"description", "the gain (speaker volume)"},
                   {"minimum", 0},
                   {"maximum", MAX_GAIN}}},
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
                 {"starting_velocity",
                  {{"type", "number"},
                   {"description", "the starting velocity (note force)"},
                   {"minimum", 0},
                   {"maximum", MAX_VELOCITY}}},
                 {"chords", get_chords_schema()}}}}));
  try {
    song_validator.validate(json_song);
  } catch (const std::exception &error) {
    QMessageBox::warning(this, tr("Schema error"), error.what());
    return;
  }

  auto gain_value = get_json_value(json_song, "gain");
  Q_ASSERT(gain_value.is_number());
  Q_ASSERT(gain_editor_pointer != nullptr);
  gain_editor_pointer->setValue(gain_value.get<double>());

  Q_ASSERT(starting_key_editor_pointer != nullptr);
  starting_key_editor_pointer->setValue(
      get_json_double(json_song, "starting_key"));

  Q_ASSERT(starting_velocity_editor_pointer != nullptr);
  starting_velocity_editor_pointer->setValue(
      get_json_double(json_song, "starting_velocity"));

  Q_ASSERT(starting_tempo_editor_pointer != nullptr);
  starting_tempo_editor_pointer->setValue(
      get_json_double(json_song, "starting_tempo"));

  const auto &starting_instrument_value =
      get_json_value(json_song, "starting_instrument");
  Q_ASSERT(starting_instrument_value.is_string());
  Q_ASSERT(starting_instrument_editor_pointer != nullptr);
  starting_instrument_editor_pointer->setValue(
      get_instrument_pointer(starting_instrument_value.get<std::string>()));

  Q_ASSERT(chords_view_pointer != nullptr);
  auto *chords_model_pointer = chords_view_pointer->chords_model_pointer;
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->delete_all_chords();

  if (json_song.contains("chords")) {
    chords_model_pointer->append_json_chords(json_song["chords"]);
  }

  current_file = filename;

  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->clear();
  undo_stack_pointer->setClean();
}

void SongEditor::save_as_file(const QString &filename) {
  std::ofstream file_io(filename.toStdString().c_str());

  nlohmann::json json_song;
  json_song["gain"] = gain;
  json_song["starting_key"] = starting_key;
  json_song["starting_tempo"] = starting_tempo;
  json_song["starting_velocity"] = starting_velocity;
  json_song["starting_instrument"] =
      starting_instrument_pointer->instrument_name;

  Q_ASSERT(chords_view_pointer != nullptr);
  auto *chords_model_pointer = chords_view_pointer->chords_model_pointer;
  Q_ASSERT(chords_model_pointer != nullptr);
  if (chords_model_pointer->get_number_of_chords() > 0) {
    json_song["chords"] = chords_model_pointer->to_json();
  }

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
  auto file_result = fluid_settings_setstr(settings_pointer, "audio.file.name",
                                           output_file.toStdString().c_str());
  Q_ASSERT(file_result == FLUID_OK);

  auto unlock_result =
      fluid_settings_setint(settings_pointer, "synth.lock-memory", 0);
  Q_ASSERT(unlock_result == FLUID_OK);

  Q_ASSERT(chords_view_pointer != nullptr);
  auto *chords_model_pointer = chords_view_pointer->chords_model_pointer;
  Q_ASSERT(chords_model_pointer != nullptr);

  Q_ASSERT(sequencer_pointer != nullptr);
  auto finished = false;
  auto finished_timer_id = fluid_sequencer_register_client(
      sequencer_pointer, "finished timer",
      [](unsigned int /*time*/, fluid_event_t * /*event*/,
         fluid_sequencer_t * /*seq*/, void *data_pointer) {
        auto *finished_pointer = static_cast<bool *>(data_pointer);
        Q_ASSERT(finished_pointer != nullptr);
        *finished_pointer = true;
      },
      &finished);
  Q_ASSERT(finished_timer_id >= 0);

  initialize_play();
  auto final_time = play_chords(0, chords_model_pointer->get_number_of_chords(),
                                START_END_MILLISECONDS);

  Q_ASSERT(event_pointer != nullptr);
  fluid_event_set_dest(event_pointer, finished_timer_id);
  fluid_event_timer(event_pointer, nullptr);
  send_event_at(final_time + START_END_MILLISECONDS);

  Q_ASSERT(synth_pointer != nullptr);
  auto *renderer_pointer = new_fluid_file_renderer(synth_pointer);
  Q_ASSERT(renderer_pointer != nullptr);
  while (!finished) {
    auto process_result = fluid_file_renderer_process_block(renderer_pointer);
    Q_ASSERT(process_result == FLUID_OK);
  }
  delete_fluid_file_renderer(renderer_pointer);

  fluid_event_set_dest(event_pointer, sequencer_id);
  auto lock_result =
      fluid_settings_setint(settings_pointer, "synth.lock-memory", 1);
  Q_ASSERT(lock_result == FLUID_OK);
  start_real_time();
}
