#include "justly/SongEditor.hpp"

#include <fluidsynth.h>             // for fluid_sequencer_send_at
#include <qabstractitemdelegate.h>  // for QAbstractItemDelegate
#include <qabstractitemmodel.h>     // for QModelIndex, QAbstra...
#include <qabstractitemview.h>      // for QAbstractItemView
#include <qaction.h>                // for QAction
#include <qassert.h>                // for Q_ASSERT
#include <qbytearray.h>             // for QByteArray
#include <qclipboard.h>             // for QClipboard
#include <qcombobox.h>              // for QComboBox
#include <qcoreapplication.h>       // for QCoreApplication
#include <qdir.h>                   // for QDir
#include <qdockwidget.h>            // for QDockWidget, QDockWi...
#include <qfiledialog.h>            // for QFileDialog, QFileDi...
#include <qformlayout.h>            // for QFormLayout
#include <qframe.h>                 // for QFrame
#include <qguiapplication.h>        // for QGuiApplication
#include <qitemeditorfactory.h>     // for QStandardItemEditorC...
#include <qitemselectionmodel.h>    // for QItemSelectionModel
#include <qkeysequence.h>           // for QKeySequence, QKeySe...
#include <qlineedit.h>              // for QLineEdit
#include <qlist.h>                  // for QList
#include <qlogging.h>               // for qWarning
#include <qmenu.h>                  // for QMenu
#include <qmenubar.h>               // for QMenuBar
#include <qmessagebox.h>            // for QMessageBox, QMessag...
#include <qmetaobject.h>            // for QMetaProperty
#include <qmetatype.h>              // for qMetaTypeId
#include <qmimedata.h>              // for QMimeData
#include <qnamespace.h>             // for ItemDataRole, Horizo...
#include <qobject.h>                // for QObject
#include <qobjectdefs.h>            // for QMetaObject
#include <qrect.h>                  // for QRect
#include <qscreen.h>                // for QScreen
#include <qsize.h>                  // for QSize
#include <qsizepolicy.h>            // for QSizePolicy, QSizePo...
#include <qslider.h>                // for QSlider
#include <qspinbox.h>               // for QDoubleSpinBox
#include <qstandardpaths.h>         // for QStandardPaths, QSta...
#include <qstring.h>                // for QString
#include <qstyleoption.h>           // for QStyleOptionViewItem
#include <qthread.h>                // for QThread
#include <qundostack.h>             // for QUndoStack
#include <qwidget.h>                // for QWidget

#include <cmath>                  // for log2, round
#include <cstddef>                // for size_t
#include <cstdint>                // for int16_t, uint64_t
#include <filesystem>             // for exists
#include <fstream>                // IWYU pragma: keep
#include <initializer_list>       // for initializer_list
#include <iomanip>                // for operator<<, setw
#include <map>                    // for operator!=, operator==
#include <memory>                 // for make_unique, __uniqu...
#include <nlohmann/json.hpp>      // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>  // for json
#include <sstream>                // IWYU pragma: keep
#include <string>                 // for basic_string, string
#include <thread>                 // for thread
#include <vector>                 // for vector

#include "cell_editors/InstrumentEditor.hpp"     // for InstrumentEditor
#include "cell_editors/IntervalEditor.hpp"       // for IntervalEditor
#include "cell_editors/RationalEditor.hpp"       // for RationalEditor
#include "changes/InsertRemoveChange.hpp"        // for InsertRemoveChange
#include "changes/StartingInstrumentChange.hpp"  // for StartingInstrumentCh...
#include "changes/StartingKeyChange.hpp"         // for StartingKeyChange
#include "changes/StartingTempoChange.hpp"       // for StartingTempoChange
#include "changes/StartingVolumeChange.hpp"      // for StartingVolumeChange
#include "json/JsonErrorHandler.hpp"             // for show_parse_error
#include "json/schemas.hpp"                      // for verify_json_song
#include "justly/Chord.hpp"                      // for Chord
#include "justly/Instrument.hpp"                 // for Instrument, get_inst...
#include "justly/Interval.hpp"                   // for Interval
#include "justly/Note.hpp"                       // for Note
#include "justly/Rational.hpp"                   // for Rational
#include "justly/Song.hpp"                       // for Song
#include "models/ChordsModel.hpp"                // for ChordsModel, to_pare...
#include "other/ChordsView.hpp"                  // for ChordsView
#include "other/TreeSelector.hpp"                // for TreeSelector
#include "other/instruments.hpp"                 // for get_all_instruments
#include "other/private_constants.hpp"           // for MAX_STARTING_KEY

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
const auto PERCENT = 100;
const auto DEFAULT_PLAYBACK_VOLUME = 6.0F;
const auto NUMBER_OF_MIDI_CHANNELS = 16;
const auto MAX_GAIN = 10.0;

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

void SongEditor::update_actions() {
  Q_ASSERT(tree_selector_pointer != nullptr);
  auto selected_row_indexes = tree_selector_pointer->selectedRows();

  auto any_selected = !(selected_row_indexes.isEmpty());
  auto selected_level = root_level;
  auto empty_chord_selected = false;
  if (any_selected) {
    auto &first_index = selected_row_indexes[0];
    selected_level = get_level(first_index);
    Q_ASSERT(chords_model_pointer != nullptr);
    empty_chord_selected = selected_level == chord_level &&
                           selected_row_indexes.size() == 1 &&
                           chords_model_pointer->rowCount(first_index) == 0;
  }
  auto no_chords = song.chord_pointers.empty();
  auto can_paste = selected_level != root_level && selected_level == copy_level;

  Q_ASSERT(copy_action_pointer != nullptr);
  copy_action_pointer->setEnabled(any_selected);

  Q_ASSERT(insert_before_action_pointer != nullptr);
  insert_before_action_pointer->setEnabled(any_selected);

  Q_ASSERT(insert_after_action_pointer != nullptr);
  insert_after_action_pointer->setEnabled(any_selected);

  Q_ASSERT(insert_before_action_pointer != nullptr);
  insert_before_action_pointer->setEnabled(any_selected);

  Q_ASSERT(remove_action_pointer != nullptr);
  remove_action_pointer->setEnabled(any_selected);

  Q_ASSERT(play_action_pointer != nullptr);
  play_action_pointer->setEnabled(any_selected);

  Q_ASSERT(insert_into_action_pointer != nullptr);
  insert_into_action_pointer->setEnabled(no_chords || empty_chord_selected);

  Q_ASSERT(paste_before_action_pointer != nullptr);
  paste_before_action_pointer->setEnabled(can_paste);

  Q_ASSERT(paste_after_action_pointer != nullptr);
  paste_after_action_pointer->setEnabled(can_paste);

  Q_ASSERT(paste_into_action_pointer != nullptr);
  paste_into_action_pointer->setEnabled(
      (no_chords && copy_level == chord_level) ||
      (empty_chord_selected && copy_level == note_level));

  Q_ASSERT(save_action_pointer != nullptr);
  Q_ASSERT(undo_stack_pointer != nullptr);
  save_action_pointer->setEnabled(!undo_stack_pointer->isClean() &&
                                  !current_file.empty());
}

void SongEditor::paste(int first_child_number,
                       const QModelIndex &parent_index) {
  auto *clipboard_pointer = QGuiApplication::clipboard();

  Q_ASSERT(clipboard_pointer != nullptr);
  const QMimeData *mime_data_pointer = clipboard_pointer->mimeData();

  Q_ASSERT(mime_data_pointer != nullptr);
  if (mime_data_pointer->hasFormat("application/json")) {
    paste_text(first_child_number,
               mime_data_pointer->data("application/json").toStdString(),
               parent_index);
  }
}

void SongEditor::open() {
  Q_ASSERT(undo_stack_pointer != nullptr);
  if (undo_stack_pointer->isClean() ||
      QMessageBox::question(nullptr, tr("Unsaved changes"),
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
}

void SongEditor::save_as() {
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
}

void SongEditor::export_wav() {
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
}

void SongEditor::initialize_controls() {
  Q_ASSERT(starting_key_editor_pointer != nullptr);
  starting_key_editor_pointer->setValue(song.starting_key);

  Q_ASSERT(starting_volume_editor_pointer != nullptr);
  starting_volume_editor_pointer->setValue(song.starting_volume);

  Q_ASSERT(starting_tempo_editor_pointer != nullptr);
  starting_tempo_editor_pointer->setValue(song.starting_tempo);

  Q_ASSERT(starting_instrument_editor_pointer != nullptr);
  starting_instrument_editor_pointer->setValue(
      song.starting_instrument_pointer);
}

auto SongEditor::beat_time() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}

auto SongEditor::has_real_time() const -> bool {
  return audio_driver_pointer != nullptr;
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
  #endif
  if (audio_driver_pointer == nullptr) {
    qWarning("Cannot start audio driver \"%s\"", driver.c_str());
  }
}

void SongEditor::initialize_play() {
  current_key = song.starting_key;
  current_volume = song.starting_volume / PERCENT;
  current_tempo = song.starting_tempo;

  Q_ASSERT(sequencer_pointer != nullptr);
  starting_time = fluid_sequencer_get_tick(sequencer_pointer);
  current_time = starting_time;
  current_instrument_pointer = song.starting_instrument_pointer;
}

void SongEditor::modulate(const Chord *chord_pointer) {
  Q_ASSERT(chord_pointer != nullptr);
  current_key = current_key * chord_pointer->interval.ratio();
  current_volume = current_volume * chord_pointer->volume_ratio.ratio();
  current_tempo = current_tempo * chord_pointer->tempo_ratio.ratio();
  const auto &chord_instrument_pointer = chord_pointer->instrument_pointer;
  Q_ASSERT(chord_instrument_pointer != nullptr);
  if (!chord_instrument_pointer->instrument_name.empty()) {
    current_instrument_pointer = chord_instrument_pointer;
  }
}

auto SongEditor::play_notes(size_t chord_index, const Chord *chord_pointer,
                            size_t first_note_index, size_t number_of_notes)
    -> unsigned int {
  Q_ASSERT(chord_pointer != nullptr);
  const auto &note_pointers = chord_pointer->note_pointers;
  unsigned int final_time = 0;
  for (auto note_index = first_note_index;
       note_index < first_note_index + number_of_notes;
       note_index = note_index + 1) {
    Q_ASSERT(note_index < note_pointers.size());
    const auto &note_pointer = note_pointers[note_index];

    Q_ASSERT(note_pointer != nullptr);
    const auto &note_instrument_pointer = note_pointer->instrument_pointer;

    Q_ASSERT(note_instrument_pointer != nullptr);
    const auto &instrument_pointer =
        (note_instrument_pointer->instrument_name.empty()
             ? current_instrument_pointer
             : note_instrument_pointer);

    Q_ASSERT(CONCERT_A_FREQUENCY != 0);
    auto key_float = HALFSTEPS_PER_OCTAVE *
                         log2(current_key * note_pointer->interval.ratio() /
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
      std::stringstream warning_message;
      warning_message << "Out of midi channels for chord " << chord_index + 1
                      << ", note " << note_index + 1 << ". Not playing note.";
      QMessageBox::warning(nullptr, QObject::tr("Playback error error"),
                           QObject::tr(warning_message.str().c_str()));
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

      auto new_volume = current_volume * note_pointer->volume_ratio.ratio();
      if (new_volume > 1) {
        std::stringstream warning_message;
        warning_message << "Volume exceeds 100% for chord " << chord_index + 1
                        << ", note " << note_index + 1
                        << ". Playing with 100% volume.";
        QMessageBox::warning(nullptr, QObject::tr("Playback error error"),
                             QObject::tr(warning_message.str().c_str()));
        new_volume = 1;
      }

      fluid_event_noteon(event_pointer, channel_number, int_closest_key,
                         static_cast<int16_t>(new_volume * MAX_VELOCITY));
      fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                              int_current_time + 2, 1);

      auto time_step = (beat_time() * note_pointer->beats.ratio() *
                        note_pointer->tempo_ratio.ratio()) *
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
  const auto &chord_pointers = song.chord_pointers;
  current_time = current_time + wait_frames;
  unsigned int final_time = 0;
  for (auto chord_index = first_chord_index;
       chord_index < first_chord_index + number_of_chords;
       chord_index = chord_index + 1) {
    Q_ASSERT(chord_index < chord_pointers.size());
    const auto *chord_pointer = chord_pointers[chord_index].get();

    Q_ASSERT(chord_pointer != nullptr);
    modulate(chord_pointer);
    auto end_time = play_notes(chord_index, chord_pointer, 0,
                               chord_pointer->note_pointers.size());
    if (end_time > final_time) {
      final_time = end_time;
    }
    auto time_step =
        (beat_time() * chord_pointer->beats.ratio()) * MILLISECONDS_PER_SECOND;
    current_time = current_time + static_cast<unsigned int>(time_step);
  }
  return final_time;
}

SongEditor::SongEditor(QWidget *parent_pointer, Qt::WindowFlags flags)
    : QMainWindow(parent_pointer, flags),
      copy_level(root_level),
      current_folder(
          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
              .toStdString()),
      playback_volume_editor_pointer(new QSlider(Qt::Horizontal, this)),
      starting_instrument_editor_pointer(new InstrumentEditor(this, false)),
      starting_key_editor_pointer(new QDoubleSpinBox(this)),
      starting_volume_editor_pointer(new QDoubleSpinBox(this)),
      starting_tempo_editor_pointer(new QDoubleSpinBox(this)),
      chords_view_pointer(new ChordsView(this)),
      undo_stack_pointer(new QUndoStack(this)),
      insert_before_action_pointer(new QAction(tr("&Before"), this)),
      insert_after_action_pointer(new QAction(tr("&After"), this)),
      insert_into_action_pointer(new QAction(tr("&Into"), this)),
      remove_action_pointer(new QAction(tr("&Remove"), this)),
      copy_action_pointer(new QAction(tr("&Copy"), this)),
      paste_before_action_pointer(new QAction(tr("&Before"), this)),
      paste_after_action_pointer(new QAction(tr("&After"), this)),
      paste_into_action_pointer(new QAction(tr("&Into"), this)),
      save_action_pointer(new QAction(tr("&Save"), this)),
      play_action_pointer(new QAction(tr("&Play selection"), this)),
      channel_schedules(std::vector<unsigned int>(NUMBER_OF_MIDI_CHANNELS, 0)),
      playback_volume(DEFAULT_PLAYBACK_VOLUME),
      current_instrument_pointer(get_instrument_pointer("")) {
  auto *factory = std::make_unique<QItemEditorFactory>().release();
  factory->registerEditor(
      qMetaTypeId<Rational>(),
      std::make_unique<QStandardItemEditorCreator<RationalEditor>>().release());
  factory->registerEditor(
      qMetaTypeId<const Instrument *>(),
      std::make_unique<QStandardItemEditorCreator<InstrumentEditor>>()
          .release());
  factory->registerEditor(
      qMetaTypeId<Interval>(),
      std::make_unique<QStandardItemEditorCreator<IntervalEditor>>().release());
  factory->registerEditor(
      qMetaTypeId<QString>(),
      std::make_unique<QStandardItemEditorCreator<QLineEdit>>().release());
  QItemEditorFactory::setDefaultFactory(factory);

  chords_model_pointer =
      std::make_unique<ChordsModel>(&song, undo_stack_pointer, this).release();

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
  connect(open_action_pointer, &QAction::triggered, this, &SongEditor::open);
  open_action_pointer->setShortcuts(QKeySequence::Open);

  file_menu_pointer->addSeparator();

  save_action_pointer->setShortcuts(QKeySequence::Save);
  connect(save_action_pointer, &QAction::triggered, this, &SongEditor::save);
  file_menu_pointer->addAction(save_action_pointer);
  save_action_pointer->setEnabled(false);

  auto *save_as_action_pointer =
      std::make_unique<QAction>(tr("&Save As..."), file_menu_pointer).release();
  save_as_action_pointer->setShortcuts(QKeySequence::SaveAs);
  connect(save_as_action_pointer, &QAction::triggered, this,
          &SongEditor::save_as);
  file_menu_pointer->addAction(save_as_action_pointer);
  save_as_action_pointer->setEnabled(true);

  auto *export_as_action_pointer =
      std::make_unique<QAction>(tr("&Export recording"), file_menu_pointer)
          .release();
  connect(export_as_action_pointer, &QAction::triggered, this,
          &SongEditor::export_wav);
  file_menu_pointer->addAction(export_as_action_pointer);

  menu_bar_pointer->addMenu(file_menu_pointer);

  auto *edit_menu_pointer =
      std::make_unique<QMenu>(tr("&Edit"), this).release();

  auto *undo_action_pointer =
      undo_stack_pointer->createUndoAction(edit_menu_pointer);
  Q_ASSERT(undo_action_pointer != nullptr);
  undo_action_pointer->setShortcuts(QKeySequence::Undo);
  edit_menu_pointer->addAction(undo_action_pointer);

  auto *redo_action_pointer =
      undo_stack_pointer->createRedoAction(edit_menu_pointer);
  Q_ASSERT(redo_action_pointer != nullptr);
  redo_action_pointer->setShortcuts(QKeySequence::Redo);
  edit_menu_pointer->addAction(redo_action_pointer);

  edit_menu_pointer->addSeparator();

  copy_action_pointer->setEnabled(false);
  copy_action_pointer->setShortcuts(QKeySequence::Copy);
  connect(copy_action_pointer, &QAction::triggered, this,
          &SongEditor::copy_selected);
  edit_menu_pointer->addAction(copy_action_pointer);

  auto *paste_menu_pointer =
      std::make_unique<QMenu>(tr("&Paste"), edit_menu_pointer).release();

  paste_before_action_pointer->setEnabled(false);
  connect(paste_before_action_pointer, &QAction::triggered, this,
          &SongEditor::paste_before);
  paste_menu_pointer->addAction(paste_before_action_pointer);

  paste_after_action_pointer->setEnabled(false);
  paste_after_action_pointer->setShortcuts(QKeySequence::Paste);
  connect(paste_after_action_pointer, &QAction::triggered, this,
          &SongEditor::paste_after);
  paste_menu_pointer->addAction(paste_after_action_pointer);

  paste_into_action_pointer->setEnabled(false);
  connect(paste_into_action_pointer, &QAction::triggered, this,
          &SongEditor::paste_into);
  paste_menu_pointer->addAction(paste_into_action_pointer);

  edit_menu_pointer->addMenu(paste_menu_pointer);

  edit_menu_pointer->addSeparator();

  auto *insert_menu_pointer =
      std::make_unique<QMenu>(tr("&Insert"), edit_menu_pointer).release();

  edit_menu_pointer->addMenu(insert_menu_pointer);

  insert_before_action_pointer->setEnabled(false);
  connect(insert_before_action_pointer, &QAction::triggered, this,
          &SongEditor::insert_before);
  insert_menu_pointer->addAction(insert_before_action_pointer);

  insert_after_action_pointer->setEnabled(false);
  insert_after_action_pointer->setShortcuts(QKeySequence::InsertLineSeparator);
  connect(insert_after_action_pointer, &QAction::triggered, this,
          &SongEditor::insert_after);
  insert_menu_pointer->addAction(insert_after_action_pointer);

  insert_into_action_pointer->setEnabled(true);
  insert_into_action_pointer->setShortcuts(QKeySequence::AddTab);
  connect(insert_into_action_pointer, &QAction::triggered, this,
          &SongEditor::insert_into);
  insert_menu_pointer->addAction(insert_into_action_pointer);

  remove_action_pointer->setEnabled(false);
  remove_action_pointer->setShortcuts(QKeySequence::Delete);
  connect(remove_action_pointer, &QAction::triggered, this,
          &SongEditor::remove_selected);
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
  connect(play_action_pointer, &QAction::triggered, this,
          &SongEditor::play_selected);
  play_menu_pointer->addAction(play_action_pointer);

  auto *stop_playing_action_pointer =
      std::make_unique<QAction>(tr("&Stop playing"), play_menu_pointer)
          .release();
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
  connect(playback_volume_editor_pointer, &QSlider::valueChanged, this,
          [this](int new_value) {
            Q_ASSERT(synth_pointer != nullptr);
            fluid_synth_set_gain(
                synth_pointer,
                static_cast<float>(1.0 * new_value / PERCENT * MAX_GAIN));
          });
  controls_form_pointer->addRow(tr("&Playback volume:"),
                                playback_volume_editor_pointer);

  connect(starting_instrument_editor_pointer, &QComboBox::currentIndexChanged,
          this, [this](size_t new_index) {
            const auto &all_instruments = get_all_instruments();
            Q_ASSERT(new_index < all_instruments.size());
            undo_stack_pointer->push(std::make_unique<StartingInstrumentChange>(
                                         this, song.starting_instrument_pointer,
                                         &(all_instruments[new_index]))
                                         .release());
          });
  controls_form_pointer->addRow(tr("Starting &instrument:"),
                                starting_instrument_editor_pointer);

  starting_key_editor_pointer->setMinimum(MIN_STARTING_KEY);
  starting_key_editor_pointer->setMaximum(MAX_STARTING_KEY);
  starting_key_editor_pointer->setDecimals(1);
  starting_key_editor_pointer->setSuffix(" hz");
  connect(starting_key_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](int new_value) {
            undo_stack_pointer->push(std::make_unique<StartingKeyChange>(
                                         this, song.starting_key, new_value)
                                         .release());
          });
  controls_form_pointer->addRow(tr("Starting &key:"),
                                starting_key_editor_pointer);

  starting_volume_editor_pointer->setMinimum(1);
  starting_volume_editor_pointer->setMaximum(MAX_STARTING_VOLUME);
  starting_volume_editor_pointer->setDecimals(1);
  starting_volume_editor_pointer->setSuffix("%");
  connect(starting_volume_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](int new_value) {
            undo_stack_pointer->push(std::make_unique<StartingVolumeChange>(
                                         this, song.starting_volume, new_value)
                                         .release());
          });
  controls_form_pointer->addRow(tr("Starting &volume:"),
                                starting_volume_editor_pointer);

  starting_tempo_editor_pointer->setMinimum(MIN_STARTING_TEMPO);
  starting_tempo_editor_pointer->setMaximum(MAX_STARTING_TEMPO);
  starting_tempo_editor_pointer->setDecimals(1);
  starting_tempo_editor_pointer->setSuffix(" bpm");
  connect(starting_tempo_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](int new_value) {
            undo_stack_pointer->push(std::make_unique<StartingTempoChange>(
                                         this, song.starting_tempo, new_value)
                                         .release());
          });
  controls_form_pointer->addRow(tr("Starting &tempo:"),
                                starting_tempo_editor_pointer);

  controls_pointer->setLayout(controls_form_pointer);

  dock_widget_pointer->setWidget(controls_pointer);
  dock_widget_pointer->setFeatures(QDockWidget::NoDockWidgetFeatures);
  addDockWidget(Qt::LeftDockWidgetArea, dock_widget_pointer);

  tree_selector_pointer =
      std::make_unique<TreeSelector>(chords_model_pointer).release();
  chords_view_pointer->setModel(chords_model_pointer);
  chords_view_pointer->setSelectionModel(tree_selector_pointer);

  connect(tree_selector_pointer, &QItemSelectionModel::selectionChanged, this,
          &SongEditor::update_actions);

  setWindowTitle("Justly");
  setCentralWidget(chords_view_pointer);

  connect(undo_stack_pointer, &QUndoStack::cleanChanged, this,
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

  Q_ASSERT(settings_pointer != nullptr);
  auto cores = std::thread::hardware_concurrency();
  if (cores > 0) {
    fluid_settings_setint(settings_pointer, "synth.cpu-cores",
                          static_cast<int>(cores));
  }
  if (VERBOSE_FLUIDSYNTH) {
    fluid_settings_setint(settings_pointer, "synth.verbose", 1);
  }

  synth_pointer = new_fluid_synth(settings_pointer);
  Q_ASSERT(synth_pointer != nullptr);

  // need a synth to do this
  set_playback_volume(playback_volume);

  Q_ASSERT(sequencer_pointer != nullptr);
  sequencer_id =
      fluid_sequencer_register_fluidsynth(sequencer_pointer, synth_pointer);
  Q_ASSERT(sequencer_id != -1);

  auto soundfont_file = QDir(QCoreApplication::applicationDirPath())
                            .filePath(SOUNDFONT_RELATIVE_PATH)
                            .toStdString();
  Q_ASSERT(std::filesystem::exists(soundfont_file));

  auto maybe_soundfont_id =
      fluid_synth_sfload(synth_pointer, soundfont_file.c_str(), 1);
  Q_ASSERT(maybe_soundfont_id != -1);
  soundfont_id = maybe_soundfont_id;

  fluid_event_set_dest(event_pointer, sequencer_id);

  start_real_time();
  initialize_controls();
  undo_stack_pointer->clear();
  undo_stack_pointer->setClean();
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

auto SongEditor::get_current_file() const -> const std::string & {
  return current_file;
}

auto SongEditor::get_selected_rows() const -> QModelIndexList {
  Q_ASSERT(tree_selector_pointer != nullptr);
  return tree_selector_pointer->selectedRows();
}

auto SongEditor::get_index(int parent_number, int child_number,
                           NoteChordField column_number) const -> QModelIndex {
  Q_ASSERT(chords_model_pointer != nullptr);
  return chords_model_pointer->get_index(parent_number, child_number,
                                         column_number);
}

void SongEditor::select_index(const QModelIndex index) {
  Q_ASSERT(tree_selector_pointer != nullptr);
  tree_selector_pointer->select(
      index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

void SongEditor::select_indices(const QModelIndex first_index,
                                const QModelIndex last_index) {
  Q_ASSERT(tree_selector_pointer != nullptr);
  tree_selector_pointer->select(
      QItemSelection(first_index, last_index),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

void SongEditor::clear_selection() {
  Q_ASSERT(tree_selector_pointer != nullptr);
  tree_selector_pointer->select(QModelIndex(), QItemSelectionModel::Clear);
}

auto SongEditor::get_number_of_children(int parent_index) -> size_t {
  return song.get_number_of_children(parent_index);
}

auto SongEditor::get_header_data(NoteChordField column_number,
                                 Qt::Orientation orientation,
                                 Qt::ItemDataRole role) const -> QVariant {
  Q_ASSERT(chords_model_pointer != nullptr);
  return chords_model_pointer->headerData(column_number, orientation, role);
}

auto SongEditor::get_row_count(QModelIndex parent_index) const -> int {
  Q_ASSERT(chords_model_pointer != nullptr);
  return chords_model_pointer->rowCount(parent_index);
};
auto SongEditor::get_column_count(QModelIndex parent_index) const -> int {
  Q_ASSERT(chords_model_pointer != nullptr);
  return chords_model_pointer->columnCount(parent_index);
};

auto SongEditor::get_parent_index(QModelIndex index) const -> QModelIndex {
  Q_ASSERT(chords_model_pointer != nullptr);
  return chords_model_pointer->parent(index);
};

auto SongEditor::get_flags(QModelIndex index) const -> Qt::ItemFlags {
  Q_ASSERT(chords_model_pointer != nullptr);
  return chords_model_pointer->flags(index);
}

auto SongEditor::get_data(QModelIndex index, Qt::ItemDataRole role)
    -> QVariant {
  Q_ASSERT(chords_model_pointer != nullptr);
  return chords_model_pointer->data(index, role);
};

auto SongEditor::set_data(QModelIndex index, const QVariant &new_value,
                          Qt::ItemDataRole role) -> bool {
  Q_ASSERT(chords_model_pointer != nullptr);
  return chords_model_pointer->setData(index, new_value, role);
};

auto SongEditor::create_editor(QModelIndex index) -> QWidget * {
  Q_ASSERT(chords_model_pointer != nullptr);

  auto *my_delegate_pointer = chords_view_pointer->itemDelegate();
  Q_ASSERT(my_delegate_pointer != nullptr);

  auto *cell_editor_pointer = my_delegate_pointer->createEditor(
      chords_view_pointer->viewport(), QStyleOptionViewItem(), index);
  Q_ASSERT(cell_editor_pointer != nullptr);

  my_delegate_pointer->setEditorData(cell_editor_pointer, index);

  return cell_editor_pointer;
}

void SongEditor::set_editor(QWidget *cell_editor_pointer, QModelIndex index,
                            const QVariant &new_value) {
  Q_ASSERT(chords_view_pointer != nullptr);
  auto *my_delegate_pointer = chords_view_pointer->itemDelegate();

  Q_ASSERT(cell_editor_pointer != nullptr);
  const auto *cell_editor_meta_object = cell_editor_pointer->metaObject();

  Q_ASSERT(cell_editor_meta_object != nullptr);
  cell_editor_pointer->setProperty(
      cell_editor_meta_object->userProperty().name(), new_value);

  Q_ASSERT(chords_model_pointer != nullptr);
  my_delegate_pointer->setModelData(cell_editor_pointer, chords_model_pointer,
                                    index);
}

auto SongEditor::get_playback_volume() const -> double {
  Q_ASSERT(synth_pointer != nullptr);
  return fluid_synth_get_gain(synth_pointer);
}

void SongEditor::set_playback_volume(double new_playback_volume) {
  Q_ASSERT(playback_volume_editor_pointer != nullptr);
  playback_volume_editor_pointer->setValue(
      static_cast<int>((1.0 * new_playback_volume) / MAX_GAIN * PERCENT));
}

auto SongEditor::get_starting_instrument() const -> const Instrument * {
  return song.starting_instrument_pointer;
}

void SongEditor::set_starting_instrument_directly(const Instrument *new_value) {
  Q_ASSERT(starting_instrument_editor_pointer != nullptr);
  if (starting_instrument_editor_pointer->value() != new_value) {
    starting_instrument_editor_pointer->blockSignals(true);
    starting_instrument_editor_pointer->setValue(new_value);
    starting_instrument_editor_pointer->blockSignals(false);
  }
  song.starting_instrument_pointer = new_value;
}

void SongEditor::set_starting_instrument_undoable(const Instrument *new_value) {
  Q_ASSERT(starting_instrument_editor_pointer != nullptr);
  if (starting_instrument_editor_pointer->value() != new_value) {
    starting_instrument_editor_pointer->setValue(new_value);
  }
}

auto SongEditor::get_starting_key() const -> double {
  return song.starting_key;
}

void SongEditor::set_starting_key_directly(double new_value) {
  Q_ASSERT(starting_key_editor_pointer != nullptr);
  if (starting_key_editor_pointer->value() != new_value) {
    starting_key_editor_pointer->blockSignals(true);
    starting_key_editor_pointer->setValue(new_value);
    starting_key_editor_pointer->blockSignals(false);
  }
  song.starting_key = new_value;
}

void SongEditor::set_starting_key_undoable(double new_value) {
  Q_ASSERT(starting_key_editor_pointer != nullptr);
  if (starting_key_editor_pointer->value() != new_value) {
    starting_key_editor_pointer->setValue(new_value);
  }
}

auto SongEditor::get_starting_volume() const -> double {
  return song.starting_volume;
};

void SongEditor::set_starting_volume_directly(double new_value) {
  Q_ASSERT(starting_volume_editor_pointer != nullptr);
  if (starting_volume_editor_pointer->value() != new_value) {
    starting_volume_editor_pointer->blockSignals(true);
    starting_volume_editor_pointer->setValue(new_value);
    starting_volume_editor_pointer->blockSignals(false);
  }
  song.starting_volume = new_value;
}

void SongEditor::set_starting_volume_undoable(double new_value) {
  Q_ASSERT(starting_volume_editor_pointer != nullptr);
  if (starting_volume_editor_pointer->value() != new_value) {
    starting_volume_editor_pointer->setValue(new_value);
  }
}

auto SongEditor::get_starting_tempo() const -> double {
  return song.starting_tempo;
};

void SongEditor::set_starting_tempo_directly(double new_value) {
  Q_ASSERT(starting_tempo_editor_pointer != nullptr);
  if (starting_tempo_editor_pointer->value() != new_value) {
    starting_tempo_editor_pointer->blockSignals(true);
    starting_tempo_editor_pointer->setValue(new_value);
    starting_tempo_editor_pointer->blockSignals(false);
  }
  song.starting_tempo = new_value;
}

void SongEditor::set_starting_tempo_undoable(double new_value) {
  Q_ASSERT(starting_tempo_editor_pointer != nullptr);
  if (starting_tempo_editor_pointer->value() != new_value) {
    starting_tempo_editor_pointer->setValue(new_value);
  }
}

void SongEditor::undo() {
  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->undo();
}

void SongEditor::redo() {
  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->redo();
}

void SongEditor::copy_selected() {
  auto selected_row_indexes = get_selected_rows();
  Q_ASSERT(!(selected_row_indexes.empty()));
  auto first_index = selected_row_indexes[0];

  Q_ASSERT(chords_model_pointer != nullptr);
  auto parent_index = chords_model_pointer->parent(first_index);
  copy_level = get_level(first_index);
  auto *new_data_pointer = std::make_unique<QMimeData>().release();
  std::stringstream json_text;

  json_text << std::setw(4)
            << chords_model_pointer->copy(first_index.row(),
                                          selected_row_indexes.size(),
                                          to_parent_index(parent_index));

  Q_ASSERT(new_data_pointer != nullptr);
  new_data_pointer->setData("application/json",
                            QByteArray::fromStdString(json_text.str()));

  auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  clipboard_pointer->setMimeData(new_data_pointer);
  update_actions();
}

void SongEditor::paste_text(int first_child_number, const std::string &text,
                            const QModelIndex &parent_index) {
  nlohmann::json json_song;
  try {
    json_song = nlohmann::json::parse(text);
  } catch (const nlohmann::json::parse_error &parse_error) {
    show_parse_error(parse_error.what());
    return;
  }

  if (!verify_children(parent_index, json_song)) {
    return;
  }

  Q_ASSERT(undo_stack_pointer != nullptr);
  Q_ASSERT(chords_model_pointer != nullptr);
  undo_stack_pointer->push(std::make_unique<InsertRemoveChange>(
                               chords_model_pointer, first_child_number,
                               json_song, to_parent_index(parent_index), true)
                               .release());
}

void SongEditor::paste_before() {
  auto selected_row_indexes = get_selected_rows();
  Q_ASSERT(!selected_row_indexes.empty());
  const auto &first_index = selected_row_indexes[0];
  paste(first_index.row(), first_index.parent());
}

void SongEditor::paste_after() {
  auto selected_row_indexes = get_selected_rows();
  Q_ASSERT(!selected_row_indexes.empty());
  const auto &last_index =
      selected_row_indexes[selected_row_indexes.size() - 1];
  paste(last_index.row() + 1, last_index.parent());
}

void SongEditor::paste_into() {
  auto selected_row_indexes = get_selected_rows();
  paste(0,
        selected_row_indexes.empty() ? QModelIndex() : selected_row_indexes[0]);
}

void SongEditor::insert_before() {
  auto selected_row_indexes = get_selected_rows();
  Q_ASSERT(!selected_row_indexes.empty());
  const auto &first_index = selected_row_indexes[0];
  chords_model_pointer->insertRows(first_index.row(), 1, first_index.parent());
}

void SongEditor::insert_after() {
  auto selected_row_indexes = get_selected_rows();
  Q_ASSERT(!selected_row_indexes.empty());
  const auto &last_index =
      selected_row_indexes[selected_row_indexes.size() - 1];
  chords_model_pointer->insertRows(last_index.row() + 1, 1,
                                   last_index.parent());
}

void SongEditor::insert_into() {
  auto selected_row_indexes = get_selected_rows();
  chords_model_pointer->insertRows(
      0, 1,
      selected_row_indexes.empty() ? QModelIndex() : selected_row_indexes[0]);
}

void SongEditor::remove_selected() {
  auto selected_row_indexes = get_selected_rows();
  Q_ASSERT(!selected_row_indexes.empty());
  const auto &first_index = selected_row_indexes[0];
  chords_model_pointer->removeRows(
      first_index.row(), static_cast<int>(selected_row_indexes.size()),
      first_index.parent());
}

void SongEditor::open_file(const std::string &filename) {
  try {
    std::ifstream file_io(filename.c_str());
    auto json_song = nlohmann::json::parse(file_io);
    file_io.close();
    if (verify_json_song(json_song)) {
      song.load_starting_values(json_song);
      initialize_controls();

      Q_ASSERT(chords_model_pointer != nullptr);
      chords_model_pointer->load_chords(json_song);

      Q_ASSERT(undo_stack_pointer != nullptr);
      undo_stack_pointer->clear();
      undo_stack_pointer->setClean();
    }
  } catch (const nlohmann::json::parse_error &parse_error) {
    show_parse_error(parse_error.what());
    return;
  }
}

void SongEditor::save() { save_as_file(get_current_file()); }

void SongEditor::save_as_file(const std::string &filename) {
  std::ofstream file_io(filename.c_str());
  file_io << std::setw(4) << song.json();
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

  initialize_play();
  auto final_time =
      play_chords(0, song.chord_pointers.size(), START_END_MILLISECONDS);

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

void SongEditor::play_selected() {
  auto selected_row_indexes = get_selected_rows();

  Q_ASSERT(!(selected_row_indexes.empty()));
  auto first_index = selected_row_indexes[0];
  auto first_child_number = first_index.row();
  auto number_of_children = selected_row_indexes.size();

  Q_ASSERT(chords_model_pointer != nullptr);
  auto parent_number =
      to_parent_index(chords_model_pointer->parent(first_index));
  initialize_play();
  const auto &chord_pointers = song.chord_pointers;
  if (parent_number == -1) {
    for (size_t chord_index = 0;
         chord_index < static_cast<size_t>(first_child_number);
         chord_index = chord_index + 1) {
      Q_ASSERT(static_cast<size_t>(chord_index) < chord_pointers.size());
      modulate(chord_pointers[chord_index].get());
    }
    play_chords(first_child_number, number_of_children);
  } else {
    Q_ASSERT(parent_number >= 0);
    auto unsigned_parent_number = static_cast<size_t>(parent_number);
    for (size_t chord_index = 0; chord_index <= unsigned_parent_number;
         chord_index = chord_index + 1) {
      Q_ASSERT(chord_index < chord_pointers.size());
      modulate(chord_pointers[chord_index].get());
    }

    Q_ASSERT(0 <= parent_number);
    Q_ASSERT(unsigned_parent_number < chord_pointers.size());
    play_notes(parent_number, chord_pointers[parent_number].get(),
               first_child_number, number_of_children);
  }
}

void SongEditor::stop_playing() {
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
