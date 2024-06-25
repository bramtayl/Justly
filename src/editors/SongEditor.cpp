#include "justly/SongEditor.hpp"

#include <fluidsynth.h>          // for fluid_sequencer_send_at
#include <qabstractitemmodel.h>  // for QModelIndex, QAbstra...
#include <qabstractitemview.h>   // for QAbstractItemView
#include <qaction.h>             // for QAction
#include <qbytearray.h>          // for QByteArray
#include <qclipboard.h>          // for QClipboard
#include <qcombobox.h>           // for QComboBox
#include <qcontainerfwd.h>       // for QStringList
#include <qcoreapplication.h>    // for QCoreApplication
#include <qdir.h>                // for QDir
#include <qdockwidget.h>         // for QDockWidget, QDockWi...
#include <qfiledialog.h>         // for QFileDialog, QFileDi...
#include <qformlayout.h>         // for QFormLayout
#include <qframe.h>              // for QFrame
#include <qguiapplication.h>     // for QGuiApplication
#include <qitemeditorfactory.h>
#include <qitemselectionmodel.h>  // for QItemSelectionModel
#include <qkeysequence.h>         // for QKeySequence, QKeySe...
#include <qlineedit.h>
#include <qlist.h>           // for QList, QList<>::iter...
#include <qmenu.h>           // for QMenu
#include <qmenubar.h>        // for QMenuBar
#include <qmessagebox.h>     // for QMessageBox, QMessag...
#include <qmimedata.h>       // for QMimeData
#include <qnamespace.h>      // for LeftDockWidgetArea
#include <qrect.h>           // for QRect
#include <qscreen.h>         // for QScreen
#include <qsize.h>           // for QSize
#include <qsizepolicy.h>     // for QSizePolicy, QSizePo...
#include <qspinbox.h>        // for QDoubleSpinBox
#include <qstandardpaths.h>  // for QStandardPaths, QSta...
#include <qstring.h>         // for QString, qUtf8Printable
#include <qstyleditemdelegate.h>
#include <qthread.h>     // for QThread
#include <qundostack.h>  // for QUndoStack
#include <qwidget.h>     // for QWidget

#include <cmath>                  // for log2, round
#include <cstddef>                // for size_t
#include <cstdint>                // for int16_t, uint64_t
#include <fstream>                // for ofstream, ifstream
#include <initializer_list>       // for initializer_list
#include <map>                    // for operator!=, operator==
#include <memory>                 // for make_unique, __uniqu...
#include <nlohmann/json.hpp>      // for basic_json, basic_js...
#include <nlohmann/json_fwd.hpp>  // for json
#include <sstream>
#include <string>   // for allocator, string
#include <thread>   // for thread
#include <utility>  // for move
#include <vector>   // for vector

#include "changes/InsertRemoveChange.hpp"        // for InsertRemoveChange
#include "changes/StartingInstrumentChange.hpp"  // for StartingInstrumentCh...
#include "changes/StartingKeyChange.hpp"         // for StartingKeyChange
#include "changes/StartingTempoChange.hpp"       // for StartingTempoChange
#include "changes/StartingVolumeChange.hpp"      // for StartingVolumeChange
#include "editors/ChordsView.hpp"                // for ChordsView
#include "editors/InstrumentEditor.hpp"          // for InstrumentEditor
#include "editors/IntervalEditor.hpp"
#include "editors/RationalEditor.hpp"
#include "editors/TreeSelector.hpp"
#include "json/JsonErrorHandler.hpp"  // for show_parse_error
#include "json/schemas.hpp"           // for verify_json_song
#include "justly/Chord.hpp"           // for Chord
#include "justly/Instrument.hpp"      // for Instrument, get_inst...
#include "justly/Interval.hpp"        // for Interval
#include "justly/Note.hpp"            // for Note
#include "justly/Song.hpp"            // for Song, MAX_STARTING_KEY
#include "models/ChordsModel.hpp"     // for ChordsModel, get_level
#include "song/instruments.hpp"       // for get_all_instruments
#include "song/private_constants.hpp"

const auto CONCERT_A_FREQUENCY = 440;
const auto CONCERT_A_MIDI = 69;
const auto HALFSTEPS_PER_OCTAVE = 12;
const auto MAX_VELOCITY = 127;
const unsigned int MILLISECONDS_PER_SECOND = 1000;
const auto BEND_PER_HALFSTEP = 4096;
const auto ZERO_BEND_HALFSTEPS = 2;
// insert end buffer at the end of songs
const unsigned int END_BUFFER = 500;
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
  auto chords_selection = tree_selector_pointer->selectedRows();
  auto any_selected = !(chords_selection.isEmpty());
  auto selected_level = root_level;
  auto empty_chord_selected = false;
  if (any_selected) {
    auto &first_index = chords_selection[0];
    selected_level = get_level(first_index);
    empty_chord_selected = selected_level == chord_level &&
                           chords_selection.size() == 1 &&
                           chords_model_pointer->rowCount(first_index) == 0;
  }
  auto no_chords = song.chord_pointers.empty();
  auto can_paste = selected_level != root_level && selected_level == copy_level;

  copy_action_pointer->setEnabled(any_selected);
  insert_before_action_pointer->setEnabled(any_selected);
  insert_after_action_pointer->setEnabled(any_selected);
  insert_before_action_pointer->setEnabled(any_selected);
  remove_action_pointer->setEnabled(any_selected);
  play_action_pointer->setEnabled(any_selected);
  insert_into_action_pointer->setEnabled(no_chords || empty_chord_selected);

  paste_before_action_pointer->setEnabled(can_paste);
  paste_after_action_pointer->setEnabled(can_paste);
  paste_into_action_pointer->setEnabled(
      (no_chords && copy_level == chord_level) ||
      (empty_chord_selected && copy_level == note_level));

  save_action_pointer->setEnabled(!undo_stack_pointer->isClean() &&
                                  !current_file.isEmpty());
}

void SongEditor::paste(int first_child_number,
                       const QModelIndex &parent_index) {
  const QMimeData *mime_data_pointer = QGuiApplication::clipboard()->mimeData();
  if (mime_data_pointer->hasFormat("application/json")) {
    paste_text(first_child_number,
               mime_data_pointer->data("application/json").toStdString(),
               parent_index);
  }
}

void SongEditor::open() {
  if (undo_stack_pointer->isClean() ||
      QMessageBox::question(nullptr, tr("Unsaved changes"),
                            tr("Discard unsaved changes?")) ==
          QMessageBox::Yes) {
    QFileDialog dialog(this, "Open — Justly", current_folder,
                       "JSON file (*.json)");

    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setDefaultSuffix(".json");
    dialog.setFileMode(QFileDialog::ExistingFile);

    if (dialog.exec() != 0) {
      current_folder = dialog.directory().absolutePath();
      open_file(dialog.selectedFiles()[0]);
    }
  }
}

void SongEditor::save_as() {
  QFileDialog dialog(this, "Save As — Justly", current_folder,
                     "JSON file (*.json)");

  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setDefaultSuffix(".json");
  dialog.setFileMode(QFileDialog::AnyFile);

  if (dialog.exec() != 0) {
    current_folder = dialog.directory().absolutePath();
    save_as_file(dialog.selectedFiles()[0]);
  }
}

void SongEditor::export_recording() {
  QFileDialog dialog(this, "Export — Justly", current_folder,
                     "WAV file (*.wav)");
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setDefaultSuffix(".wav");
  dialog.setFileMode(QFileDialog::AnyFile);

  dialog.setLabelText(QFileDialog::Accept, "Export");

  if (dialog.exec() != 0) {
    current_folder = dialog.directory().absolutePath();
    export_to(dialog.selectedFiles()[0].toStdString());
  }
}

void SongEditor::initialize_controls() {
  starting_key_editor_pointer->setValue(song.starting_key);
  starting_volume_editor_pointer->setValue(song.starting_volume);
  starting_tempo_editor_pointer->setValue(song.starting_tempo);
  starting_instrument_editor_pointer->setValue(
      song.starting_instrument_pointer);
}

auto SongEditor::beat_time() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}

void SongEditor::start_real_time(const std::string &driver) {
  if (audio_driver_pointer != nullptr) {
    delete_fluid_audio_driver(audio_driver_pointer);
  }
  fluid_settings_setint(settings_pointer, "synth.lock-memory", 1);
  fluid_settings_setstr(settings_pointer, "audio.driver", driver.c_str());
  audio_driver_pointer =
      new_fluid_audio_driver(settings_pointer, synth_pointer);
  if (audio_driver_pointer == nullptr) {
    qWarning("Cannot find audio driver \"%s\"", driver.c_str());
  }
}

void SongEditor::initialize_play() {
  current_key = song.starting_key;
  current_volume = song.starting_volume / PERCENT;
  current_tempo = song.starting_tempo;
  starting_time = fluid_sequencer_get_tick(sequencer_pointer);
  current_time = starting_time;
  current_instrument_pointer = song.starting_instrument_pointer;
}

void SongEditor::modulate(const Chord *chord_pointer) {
  current_key = current_key * chord_pointer->interval.ratio();
  current_volume = current_volume * chord_pointer->volume_ratio.ratio();
  current_tempo = current_tempo * chord_pointer->tempo_ratio.ratio();
  const auto &chord_instrument_pointer = chord_pointer->instrument_pointer;
  if (!chord_instrument_pointer->instrument_name.empty()) {
    current_instrument_pointer = chord_instrument_pointer;
  }
}

auto SongEditor::play_notes(size_t chord_index, const Chord *chord_pointer,
                            size_t first_note_index, size_t number_of_notes)
    -> unsigned int {
  const auto &note_pointers = chord_pointer->note_pointers;
  unsigned int final_time = 0;
  for (auto note_index = first_note_index;
       note_index < first_note_index + number_of_notes;
       note_index = note_index + 1) {
    const auto &note_pointer = note_pointers[note_index];
    const auto &note_instrument_pointer = note_pointer->instrument_pointer;
    const auto &instrument_pointer =
        (note_instrument_pointer->instrument_name.empty()
             ? current_instrument_pointer
             : note_instrument_pointer);

    auto key_float = HALFSTEPS_PER_OCTAVE *
                         log2(current_key * note_pointer->interval.ratio() /
                              CONCERT_A_FREQUENCY) +
                     CONCERT_A_MIDI;
    auto closest_key = round(key_float);
    auto int_closest_key = static_cast<int16_t>(closest_key);
    auto channel_number = -1;

    for (auto channel_index = 0; channel_index < NUMBER_OF_MIDI_CHANNELS;
         channel_index = channel_index + 1) {
      if (current_time >= channel_schedules[channel_index]) {
        channel_number = channel_index;
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
      auto int_current_time = static_cast<unsigned int>(current_time);

      fluid_event_program_select(event_pointer, channel_number, soundfont_id,
                                 instrument_pointer->bank_number,
                                 instrument_pointer->preset_number);
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

      const unsigned int end_time =
          int_current_time +
          static_cast<unsigned int>((beat_time() * note_pointer->beats.ratio() *
                                     note_pointer->tempo_ratio.ratio()) *
                                    MILLISECONDS_PER_SECOND);

      fluid_event_noteoff(event_pointer, channel_number, int_closest_key);
      fluid_sequencer_send_at(sequencer_pointer, event_pointer, end_time, 1);

      channel_schedules[channel_number] = end_time;

      if (end_time > final_time) {
        final_time = end_time;
      }
    }
  }
  return final_time;
}

auto SongEditor::play_chords(size_t first_chord_index, size_t number_of_chords)
    -> unsigned int {
  const auto &chord_pointers = song.chord_pointers;
  unsigned int final_time = 0;
  for (auto chord_index = first_chord_index;
       chord_index < first_chord_index + number_of_chords;
       chord_index = chord_index + 1) {
    const auto *chord_pointer = chord_pointers[chord_index].get();
    modulate(chord_pointer);
    auto end_time = play_notes(chord_index, chord_pointer, 0,
                               chord_pointer->note_pointers.size());
    if (end_time > final_time) {
      final_time = end_time;
    }
    current_time =
        current_time +
        static_cast<unsigned int>((beat_time() * chord_pointer->beats.ratio()) *
                                  MILLISECONDS_PER_SECOND);
  }
  return final_time;
}

SongEditor::SongEditor(QWidget *parent_pointer, Qt::WindowFlags flags)
    : QMainWindow(parent_pointer, flags),
      copy_level(root_level),
      current_file(""),
      current_folder(
          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)),
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
      current_instrument_pointer(&(get_instrument(""))) {
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
          &SongEditor::export_recording);
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
            fluid_synth_set_gain(
                synth_pointer,
                static_cast<float>(1.0 * new_value / PERCENT * MAX_GAIN));
          });
  controls_form_pointer->addRow(tr("&Playback volume:"),
                                playback_volume_editor_pointer);

  connect(starting_instrument_editor_pointer, &QComboBox::currentIndexChanged,
          this, [this](size_t new_index) {
            undo_stack_pointer->push(std::make_unique<StartingInstrumentChange>(
                                         this, song.starting_instrument_pointer,
                                         &(get_all_instruments().at(new_index)))
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
  resize(sizeHint().width(),
         QGuiApplication::primaryScreen()->availableGeometry().height());

  fluid_settings_setint(settings_pointer, "synth.cpu-cores",
                        static_cast<int>(std::thread::hardware_concurrency()));
  if (VERBOSE_FLUIDSYNTH) {
    fluid_settings_setint(settings_pointer, "synth.verbose", 1);
  }

  synth_pointer = new_fluid_synth(settings_pointer);
  set_playback_volume(playback_volume);
  sequencer_id =
      fluid_sequencer_register_fluidsynth(sequencer_pointer, synth_pointer);

  soundfont_id = fluid_synth_sfload(
      synth_pointer,
      qUtf8Printable(QDir(QCoreApplication::applicationDirPath())
                         .filePath(SOUNDFONT_RELATIVE_PATH)),
      1);

  fluid_event_set_dest(event_pointer, sequencer_id);

  start_real_time();
  initialize_controls();
  undo_stack_pointer->clear();
  undo_stack_pointer->setClean();
}

SongEditor::~SongEditor() {
  undo_stack_pointer->disconnect();

  delete_fluid_audio_driver(audio_driver_pointer);
  delete_fluid_event(event_pointer);
  delete_fluid_sequencer(sequencer_pointer);
  delete_fluid_synth(synth_pointer);
  delete_fluid_settings(settings_pointer);
}

auto SongEditor::get_current_file() const -> const QString & {
  return current_file;
}

auto SongEditor::get_selected_rows() const -> QModelIndexList {
  return tree_selector_pointer->selectedRows();
}

auto SongEditor::get_index(int chord_number, int note_number,
                           NoteChordField column_number) const -> QModelIndex {
  auto root_index = QModelIndex();
  if (chord_number == -1) {
    return root_index;
  }
  if (note_number == -1) {
    return chords_model_pointer->index(chord_number, column_number, root_index);
  }
  return chords_model_pointer->index(
      note_number, column_number,
      chords_model_pointer->index(chord_number, symbol_column, root_index));
}

void SongEditor::select_index(const QModelIndex index) {
  select_indices(index, index);
}

void SongEditor::select_indices(const QModelIndex first_index,
                                const QModelIndex last_index) {
  tree_selector_pointer->select(
      QItemSelection(first_index, last_index),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

void SongEditor::clear_selection() {
  tree_selector_pointer->select(QModelIndex(), QItemSelectionModel::Clear);
}

auto SongEditor::get_number_of_children(int parent_index) -> size_t {
  return song.get_number_of_children(parent_index);
}

auto SongEditor::get_header_data(int column_number, Qt::Orientation orientation,
                                 Qt::ItemDataRole role) const -> QVariant {
  return chords_model_pointer->headerData(column_number, orientation, role);
}

auto SongEditor::get_row_count(QModelIndex parent_index) const -> int {
  return chords_model_pointer->rowCount(parent_index);
};
auto SongEditor::get_column_count(QModelIndex parent_index) const -> int {
  return chords_model_pointer->columnCount(parent_index);
};

auto SongEditor::get_parent_index(QModelIndex index) const -> QModelIndex {
  return chords_model_pointer->parent(index);
};

auto SongEditor::size_hint_for_column(int column) const -> int {
  return chords_view_pointer->sizeHintForColumn(column);
};

auto SongEditor::get_flags(QModelIndex index) const -> Qt::ItemFlags {
  return chords_model_pointer->flags(index);
}

auto SongEditor::get_data(QModelIndex index, Qt::ItemDataRole role)
    -> QVariant {
  return chords_model_pointer->data(index, role);
};

auto SongEditor::set_data(QModelIndex index, const QVariant &new_value,
                          Qt::ItemDataRole role) -> bool {
  return chords_model_pointer->setData(index, new_value, role);
};

auto SongEditor::create_editor(QModelIndex index) -> QWidget * {
  auto *my_delegate_pointer = chords_view_pointer->itemDelegate();
  auto *cell_editor_pointer = my_delegate_pointer->createEditor(
      chords_view_pointer->viewport(), QStyleOptionViewItem(), index);
  my_delegate_pointer->setEditorData(cell_editor_pointer, index);
  return cell_editor_pointer;
}

void SongEditor::set_editor(QWidget *cell_editor_pointer, QModelIndex index,
                            const QVariant &new_value) {
  auto *my_delegate_pointer = chords_view_pointer->itemDelegate();
  auto column = index.column();
  switch (column) {
    case beats_column: {
      qobject_cast<RationalEditor *>(cell_editor_pointer)
          ->set_rational(new_value.value<Rational>());
      break;
    }
    case interval_column: {
      qobject_cast<IntervalEditor *>(cell_editor_pointer)
          ->set_interval(new_value.value<Interval>());
      break;
    }
    case instrument_column: {
      qobject_cast<InstrumentEditor *>(cell_editor_pointer)
          ->setValue(new_value.value<const Instrument *>());
      break;
    }
    case volume_ratio_column: {
      qobject_cast<RationalEditor *>(cell_editor_pointer)
          ->set_rational(new_value.value<Rational>());
      break;
    }
    case words_column: {
      qobject_cast<QLineEdit *>(cell_editor_pointer)
          ->setText(new_value.toString());
      break;
    }
    case tempo_ratio_column: {
      qobject_cast<RationalEditor *>(cell_editor_pointer)
          ->set_rational(new_value.value<Rational>());
      break;
    }
    default:
      break;
  }
  my_delegate_pointer->setModelData(cell_editor_pointer, chords_model_pointer,
                                    index);
}

auto SongEditor::get_playback_volume() const -> double {
  return fluid_synth_get_gain(synth_pointer);
}

void SongEditor::set_playback_volume(double new_playback_volume) {
  playback_volume_editor_pointer->setValue(
      static_cast<int>((1.0 * new_playback_volume) / MAX_GAIN * PERCENT));
}

auto SongEditor::get_starting_instrument() const -> const Instrument * {
  return song.starting_instrument_pointer;
}

void SongEditor::set_starting_instrument_directly(const Instrument *new_value) {
  if (starting_instrument_editor_pointer->value() != new_value) {
    starting_instrument_editor_pointer->blockSignals(true);
    starting_instrument_editor_pointer->setValue(new_value);
    starting_instrument_editor_pointer->blockSignals(false);
  }
  song.starting_instrument_pointer = new_value;
}

void SongEditor::set_starting_instrument_undoable(const Instrument *new_value) {
  if (starting_instrument_editor_pointer->value() != new_value) {
    starting_instrument_editor_pointer->setValue(new_value);
  }
}

auto SongEditor::get_starting_key() const -> double {
  return song.starting_key;
}

void SongEditor::set_starting_key_directly(double new_value) {
  if (starting_key_editor_pointer->value() != new_value) {
    starting_key_editor_pointer->blockSignals(true);
    starting_key_editor_pointer->setValue(new_value);
    starting_key_editor_pointer->blockSignals(false);
  }
  song.starting_key = new_value;
}

void SongEditor::set_starting_key_undoable(double new_value) {
  if (starting_key_editor_pointer->value() != new_value) {
    starting_key_editor_pointer->setValue(new_value);
  }
}

auto SongEditor::get_starting_volume() const -> double {
  return song.starting_volume;
};

void SongEditor::set_starting_volume_directly(double new_value) {
  if (starting_volume_editor_pointer->value() != new_value) {
    starting_volume_editor_pointer->blockSignals(true);
    starting_volume_editor_pointer->setValue(new_value);
    starting_volume_editor_pointer->blockSignals(false);
  }
  song.starting_volume = new_value;
}

void SongEditor::set_starting_volume_undoable(double new_value) {
  if (starting_volume_editor_pointer->value() != new_value) {
    starting_volume_editor_pointer->setValue(new_value);
  }
}

auto SongEditor::get_starting_tempo() const -> double {
  return song.starting_tempo;
};

void SongEditor::set_starting_tempo_directly(double new_value) {
  if (starting_tempo_editor_pointer->value() != new_value) {
    starting_tempo_editor_pointer->blockSignals(true);
    starting_tempo_editor_pointer->setValue(new_value);
    starting_tempo_editor_pointer->blockSignals(false);
  }
  song.starting_tempo = new_value;
}

void SongEditor::set_starting_tempo_undoable(double new_value) {
  if (starting_tempo_editor_pointer->value() != new_value) {
    starting_tempo_editor_pointer->setValue(new_value);
  }
}

void SongEditor::undo() { undo_stack_pointer->undo(); }

void SongEditor::redo() { undo_stack_pointer->redo(); }

void SongEditor::copy_selected() {
  auto chords_selection = get_selected_rows();

  auto first_index = chords_selection[0];
  auto parent_index = chords_model_pointer->parent(first_index);
  copy_level = get_level(first_index);
  auto *new_data_pointer = std::make_unique<QMimeData>().release();
  std::stringstream json_text;
  json_text << std::setw(4)
            << chords_model_pointer->copy(
                   first_index.row(), chords_selection.size(),
                   chords_model_pointer->get_parent_number(parent_index));
  new_data_pointer->setData("application/json",
                            QByteArray::fromStdString(json_text.str()));
  QGuiApplication::clipboard()->setMimeData(new_data_pointer);
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
  undo_stack_pointer->push(
      std::make_unique<InsertRemoveChange>(
          chords_model_pointer, first_child_number, json_song,
          chords_model_pointer->get_parent_number(parent_index), true)
          .release());
}

void SongEditor::paste_before() {
  auto chords_selection = get_selected_rows();
  const auto &first_index = chords_selection[0];
  paste(first_index.row(), first_index.parent());
}

void SongEditor::paste_after() {
  auto chords_selection = get_selected_rows();
  const auto &last_index = chords_selection[chords_selection.size() - 1];
  paste(last_index.row() + 1, last_index.parent());
}

void SongEditor::paste_into() {
  auto chords_selection = get_selected_rows();
  paste(0, chords_selection.empty() ? QModelIndex() : chords_selection[0]);
}

void SongEditor::insert_before() {
  auto chords_selection = get_selected_rows();
  const auto &first_index = chords_selection[0];
  chords_model_pointer->insertRows(first_index.row(), 1, first_index.parent());
}

void SongEditor::insert_after() {
  auto chords_selection = get_selected_rows();
  const auto &last_index = chords_selection[chords_selection.size() - 1];
  chords_model_pointer->insertRows(last_index.row() + 1, 1,
                                   last_index.parent());
}

void SongEditor::insert_into() {
  auto chords_selection = get_selected_rows();
  chords_model_pointer->insertRows(
      0, 1, chords_selection.empty() ? QModelIndex() : chords_selection[0]);
}

void SongEditor::remove_selected() {
  auto chords_selection = get_selected_rows();
  const auto &first_index = chords_selection[0];
  chords_model_pointer->removeRows(first_index.row(),
                                   static_cast<int>(chords_selection.size()),
                                   first_index.parent());
}

void SongEditor::open_file(const QString &filename) {
  try {
    std::ifstream file_io(qUtf8Printable(filename));
    auto json_song = nlohmann::json::parse(file_io);
    file_io.close();
    if (verify_json_song(json_song)) {
      song.load_starting_values(json_song);
      initialize_controls();
      chords_model_pointer->load_chords(json_song);
      undo_stack_pointer->clear();
      undo_stack_pointer->setClean();
    }
  } catch (const nlohmann::json::parse_error &parse_error) {
    show_parse_error(parse_error.what());
    return;
  }
}

void SongEditor::save() { save_as_file(get_current_file()); }

void SongEditor::save_as_file(const QString &filename) {
  std::ofstream file_io(qUtf8Printable(filename));
  file_io << std::setw(4) << song.json();
  file_io.close();
  current_file = filename;
  undo_stack_pointer->setClean();
}

void SongEditor::export_to(const std::string &output_file) {
  stop_playing();

  delete_fluid_audio_driver(audio_driver_pointer);
  fluid_settings_setstr(settings_pointer, "audio.driver", "file");
  fluid_settings_setstr(settings_pointer, "audio.file.name",
                        output_file.c_str());
  fluid_settings_setint(settings_pointer, "synth.lock-memory", 0);
  initialize_play();
  auto final_time = play_chords(0, song.chord_pointers.size());
  audio_driver_pointer =
      new_fluid_audio_driver(settings_pointer, synth_pointer);
  QThread::usleep(
      static_cast<uint64_t>(final_time - starting_time + END_BUFFER) *
      MILLISECONDS_PER_SECOND);
  stop_playing();
  start_real_time();
}

void SongEditor::play_selected() {
  auto chords_selection = get_selected_rows();
  auto first_index = chords_selection[0];
  auto first_child_number = first_index.row();
  auto number_of_children = chords_selection.size();
  auto parent_number = chords_model_pointer->get_parent_number(
      chords_model_pointer->parent(first_index));
  initialize_play();
  const auto &chord_pointers = song.chord_pointers;
  if (parent_number == -1) {
    for (auto chord_index = 0; chord_index < first_child_number;
         chord_index = chord_index + 1) {
      modulate(chord_pointers[chord_index].get());
    }
    play_chords(first_child_number, number_of_children);
  } else {
    for (auto chord_index = 0; chord_index <= parent_number;
         chord_index = chord_index + 1) {
      modulate(chord_pointers[chord_index].get());
    }
    play_notes(parent_number, chord_pointers[parent_number].get(),
               first_child_number, number_of_children);
  }
}

void SongEditor::stop_playing() {
  fluid_sequencer_remove_events(sequencer_pointer, -1, -1, -1);
  for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
       channel_number = channel_number + 1) {
    fluid_event_all_sounds_off(event_pointer, channel_number);
    fluid_sequencer_send_now(sequencer_pointer, event_pointer);
  }
}

auto get_editor_data(QWidget *cell_editor_pointer, int column) -> QVariant {
  QVariant editor_value;
  switch (column) {
    case beats_column: {
      editor_value = QVariant::fromValue(
          qobject_cast<RationalEditor *>(cell_editor_pointer)->get_rational());
      break;
    }
    case interval_column: {
      editor_value = QVariant::fromValue(
          qobject_cast<IntervalEditor *>(cell_editor_pointer)->get_interval());
      break;
    }
    case instrument_column: {
      editor_value = QVariant::fromValue(
          qobject_cast<InstrumentEditor *>(cell_editor_pointer)->value());
      break;
    }
    case volume_ratio_column: {
      editor_value = QVariant::fromValue(
          qobject_cast<RationalEditor *>(cell_editor_pointer)->get_rational());
      break;
    }
    case words_column: {
      editor_value = QVariant::fromValue(
          qobject_cast<QLineEdit *>(cell_editor_pointer)->text());
      break;
    }
    case tempo_ratio_column: {
      editor_value = QVariant::fromValue(
          qobject_cast<RationalEditor *>(cell_editor_pointer)->get_rational());
      break;
    }
    default:
      break;
  }
  return editor_value;
};
