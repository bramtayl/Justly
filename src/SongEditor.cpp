#include "justly/SongEditor.h"

#include <fluidsynth.h>           // for fluid_sequencer_send_at, delete...
#include <qabstractitemmodel.h>   // for QModelIndex, QAbstractItemModel
#include <qabstractitemview.h>    // for QAbstractItemView
#include <qaction.h>              // for QAction
#include <qbytearray.h>           // for QByteArray
#include <qclipboard.h>           // for QClipboard
#include <qcombobox.h>            // for QComboBox
#include <qcontainerfwd.h>        // for QStringList
#include <qcoreapplication.h>     // for QCoreApplication
#include <qdir.h>                 // for QDir
#include <qdockwidget.h>          // for QDockWidget, QDockWidget::NoDoc...
#include <qfiledialog.h>          // for QFileDialog, QFileDialog::Accep...
#include <qformlayout.h>          // for QFormLayout
#include <qframe.h>               // for QFrame
#include <qguiapplication.h>      // for QGuiApplication
#include <qitemselectionmodel.h>  // for QItemSelectionModel, QItemSelec...
#include <qkeysequence.h>         // for QKeySequence, QKeySequence::AddTab
#include <qlist.h>                // for QList, QList<>::iterator
#include <qmenu.h>                // for QMenu
#include <qmenubar.h>             // for QMenuBar
#include <qmessagebox.h>          // for QMessageBox, QMessageBox::Yes
#include <qmimedata.h>            // for QMimeData
#include <qnamespace.h>           // for LeftDockWidgetArea, WindowFlags
#include <qrect.h>                // for QRect
#include <qscreen.h>              // for QScreen
#include <qsize.h>                // for QSize
#include <qsizepolicy.h>          // for QSizePolicy, QSizePolicy::Fixed
#include <qspinbox.h>             // for QDoubleSpinBox
#include <qstandardpaths.h>       // for QStandardPaths, QStandardPaths:...
#include <qstring.h>              // for QString
#include <qtcoreexports.h>        // for qUtf8Printable
#include <qthread.h>              // IWYU pragma: keep
#include <qundostack.h>           // for QUndoStack
#include <qvariant.h>             // for QVariant
#include <qwidget.h>              // for QWidget

#include <cmath>                  // for log2, round
#include <cstddef>                // for size_t
#include <cstdint>                // for int16_t
#include <fstream>                // for ofstream, ifstream, ostream
#include <initializer_list>       // for initializer_list
#include <map>                    // for operator!=, operator==
#include <memory>                 // for make_unique, __unique_ptr_t
#include <nlohmann/json.hpp>      // for basic_json, basic_json<>::parse...
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>                 // for string
#include <thread>                 // for thread
#include <utility>                // for move
#include <vector>                 // for vector

#include "justly/Chord.h"             // for Chord
#include "justly/ChordsModel.h"       // for ChordsModel
#include "justly/Instrument.h"        // for Instrument
#include "justly/InstrumentEditor.h"  // for InstrumentEditor
#include "justly/Interval.h"          // for Interval
#include "justly/Note.h"              // for Note
#include "justly/Song.h"              // for Song, MAX_STARTING_KEY, MAX_STA...
#include "justly/StartingField.h"     // for starting_instrument_id, startin...
#include "src/ChordsView.h"           // for ChordsView
#include "src/InsertRemoveChange.h"
#include "src/JsonErrorHandler.h"     // for JsonErrorHandler
#include "src/StartingValueChange.h"  // for StartingValueChange

const auto CONCERT_A_FREQUENCY = 440;
const auto CONCERT_A_MIDI = 69;
const auto HALFSTEPS_PER_OCTAVE = 12;
const auto MAX_VELOCITY = 127;
const auto MILLISECONDS_PER_SECOND = 1000;
const auto BEND_PER_HALFSTEP = 4096;
const auto ZERO_BEND_HALFSTEPS = 2;

SongEditor::SongEditor(QWidget *parent_pointer, Qt::WindowFlags flags)
    : QMainWindow(parent_pointer, flags),
      starting_tempo_editor_pointer(new QDoubleSpinBox(this)),
      starting_volume_editor_pointer(new QDoubleSpinBox(this)),
      starting_key_editor_pointer(new QDoubleSpinBox(this)),
      starting_instrument_editor_pointer(new InstrumentEditor(this, false)),
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
      chords_view_pointer(new ChordsView(this)),
      undo_stack_pointer(new QUndoStack(this)),
      chords_model_pointer(new ChordsModel(&song, undo_stack_pointer, this)),
      current_file(""),
      current_folder(
          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)),
      copy_level(root_level),
      current_instrument_pointer(&(Instrument::get_instrument(""))),
      synth_pointer(new_fluid_synth(settings_pointer)),
      sequencer_id(fluid_sequencer_register_fluidsynth(sequencer_pointer,
                                                       synth_pointer)) {
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
          &SongEditor::stop);
  stop_playing_action_pointer->setShortcuts(QKeySequence::Cancel);

  menu_bar_pointer->addMenu(play_menu_pointer);

  auto *controls_form_pointer =
      std::make_unique<QFormLayout>(controls_pointer).release();

  starting_key_editor_pointer->setMinimum(MIN_STARTING_KEY);
  starting_key_editor_pointer->setMaximum(MAX_STARTING_KEY);
  starting_key_editor_pointer->setDecimals(1);
  starting_key_editor_pointer->setSuffix(" hz");
  connect(starting_key_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](int new_value) {
            set_starting_value(starting_key_id, new_value);
          });
  controls_form_pointer->addRow(tr("Starting &key:"),
                                starting_key_editor_pointer);

  starting_volume_editor_pointer->setMinimum(MIN_STARTING_VOLUME);
  starting_volume_editor_pointer->setMaximum(MAX_STARTING_VOLUME);
  starting_volume_editor_pointer->setDecimals(1);
  starting_volume_editor_pointer->setSuffix("%");
  connect(starting_volume_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](int new_value) {
            set_starting_value(starting_volume_id, new_value);
          });
  controls_form_pointer->addRow(tr("Starting &volume:"),
                                starting_volume_editor_pointer);

  starting_tempo_editor_pointer->setMinimum(MIN_STARTING_TEMPO);
  starting_tempo_editor_pointer->setMaximum(MAX_STARTING_TEMPO);
  starting_tempo_editor_pointer->setDecimals(1);
  starting_tempo_editor_pointer->setSuffix(" bpm");
  connect(starting_tempo_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](int new_value) {
            set_starting_value(starting_tempo_id, new_value);
          });
  controls_form_pointer->addRow(tr("Starting &tempo:"),
                                starting_tempo_editor_pointer);

  initialize_controls();

  connect(starting_instrument_editor_pointer, &QComboBox::currentIndexChanged,
          this, &SongEditor::set_starting_instrument);
  controls_form_pointer->addRow(tr("Starting &instrument:"),
                                starting_instrument_editor_pointer);

  controls_pointer->setLayout(controls_form_pointer);

  dock_widget_pointer->setWidget(controls_pointer);
  dock_widget_pointer->setFeatures(QDockWidget::NoDockWidgetFeatures);
  addDockWidget(Qt::LeftDockWidgetArea, dock_widget_pointer);

  chords_view_pointer->setModel(chords_model_pointer);
  connect(chords_view_pointer->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &SongEditor::fix_selection);

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

  soundfont_id = fluid_synth_sfload(
      synth_pointer,
      qUtf8Printable(QDir(QCoreApplication::applicationDirPath())
                         .filePath(SOUNDFONT_RELATIVE_PATH)),
      1);

  fluid_event_set_dest(event_pointer, sequencer_id);

  start_real_time();
}

void SongEditor::copy_selected() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  auto first_index = chords_selection[0];
  auto parent_index = chords_model_pointer->parent(first_index);
  copy_level = ChordsModel::get_level(first_index);
  auto *new_data_pointer = std::make_unique<QMimeData>().release();
  new_data_pointer->setData(
      "application/json",
      QByteArray::fromStdString(
          chords_model_pointer
              ->copy_children(
                  first_index.row(), static_cast<int>(chords_selection.size()),
                  chords_model_pointer->get_chord_number(parent_index))
              .dump()));
  ;
  QGuiApplication::clipboard()->setMimeData(new_data_pointer);
  update_actions();
}

void SongEditor::play_selected() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  auto first_index = chords_selection[0];
  auto first_child_number = first_index.row();
  auto number_of_children = static_cast<int>(chords_selection.size());
  auto chord_number = chords_model_pointer->get_chord_number(
      chords_model_pointer->parent(first_index));
  initialize_player();
  const auto &chord_pointers = song.chord_pointers;
  if (chord_number == -1) {
    for (auto chord_index = 0; chord_index < first_child_number;
         chord_index = chord_index + 1) {
      modulate(chord_pointers[static_cast<size_t>(chord_index)].get());
    }
    play_chords(first_child_number, number_of_children);
  } else {
    for (auto chord_index = 0; chord_index <= chord_number;
         chord_index = chord_index + 1) {
      modulate(chord_pointers[static_cast<size_t>(chord_index)].get());
    }
    play_notes(chord_pointers[static_cast<size_t>(chord_number)].get(),
               first_child_number, number_of_children);
  }
}

void SongEditor::set_starting_value(StartingField value_type,
                                    const QVariant &new_value) {
  undo_stack_pointer->push(
      std::make_unique<StartingValueChange>(
          this, value_type, starting_value(value_type), new_value)
          .release());
}

void SongEditor::insert_before() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &first_index = chords_selection[0];
  chords_model_pointer->insertRows(first_index.row(), 1, first_index.parent());
}

void SongEditor::insert_after() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &last_index = chords_selection[chords_selection.size() - 1];
  chords_model_pointer->insertRows(last_index.row() + 1, 1,
                                   last_index.parent());
}

void SongEditor::insert_into() {
  auto chords_selection = get_selected_rows();
  chords_model_pointer->insertRows(
      0, 1, chords_selection.empty() ? QModelIndex() : chords_selection[0]);
}

void SongEditor::paste_before() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &first_index = chords_selection[0];
  paste(first_index.row(), first_index.parent());
}

void SongEditor::paste_after() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &last_index = chords_selection[chords_selection.size() - 1];
  paste(last_index.row() + 1, last_index.parent());
}

void SongEditor::paste_into() {
  auto chords_selection = get_selected_rows();
  paste(0, chords_selection.empty() ? QModelIndex() : chords_selection[0]);
}

void SongEditor::remove_selected() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &first_index = chords_selection[0];
  chords_model_pointer->removeRows(first_index.row(),
                                   static_cast<int>(chords_selection.size()),
                                   first_index.parent());
}

void SongEditor::fix_selection(const QItemSelection &selected,
                               const QItemSelection & /*deselected*/) {
  auto *selection_model_pointer = chords_view_pointer->selectionModel();
  auto all_initial_rows = selection_model_pointer->selectedRows();

  if (!all_initial_rows.isEmpty()) {
    // find an item that was already selected
    auto holdover = selection_model_pointer->selection();
    holdover.merge(selected, QItemSelectionModel::Deselect);
    // if there was holdovers, use the previous parent
    // if not, use the parent of the first new item
    const QModelIndex current_parent_index =
        holdover.isEmpty() ? all_initial_rows[0].parent()
                           : holdover[0].topLeft().parent();

    QItemSelection invalid;

    for (const QModelIndex &index : all_initial_rows) {
      if (index.parent() != current_parent_index) {
        invalid.select(index, index);
      }
    }
    if (!(invalid.isEmpty())) {
      selection_model_pointer->blockSignals(true);
      selection_model_pointer->select(
          invalid, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
      selection_model_pointer->blockSignals(false);
    }
  }

  update_actions();
}

void SongEditor::update_actions() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  auto any_selected = !(chords_selection.isEmpty());
  auto selected_level = root_level;
  auto empty_item_selected = false;
  if (any_selected) {
    auto &first_index = chords_selection[0];
    selected_level = ChordsModel::get_level(first_index);
    empty_item_selected = chords_selection.size() == 1 &&
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
  insert_into_action_pointer->setEnabled(
      no_chords || (selected_level == chord_level && empty_item_selected));

  paste_before_action_pointer->setEnabled(can_paste);
  paste_after_action_pointer->setEnabled(can_paste);
  paste_into_action_pointer->setEnabled(
      (no_chords && copy_level == chord_level) ||
      (selected_level == chord_level && empty_item_selected &&
       copy_level == note_level));

  save_action_pointer->setEnabled(!undo_stack_pointer->isClean() &&
                                  !current_file.isEmpty());
}

void SongEditor::set_starting_instrument(int new_index) {
  set_starting_value(starting_instrument_id,
                     QVariant::fromValue(&(Instrument::get_all_instruments().at(
                         static_cast<size_t>(new_index)))));
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

void SongEditor::save() { save_as_file(get_current_file()); }

void SongEditor::save_as() {
  QFileDialog dialog(this);

  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setDefaultSuffix(".json");
  dialog.setDirectory(current_folder);
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setNameFilter("JSON file (*.json)");

  if (dialog.exec() != 0) {
    current_folder = dialog.directory().absolutePath();
    save_as_file(dialog.selectedFiles()[0]);
  }
}

void SongEditor::save_as_file(const QString &filename) {
  std::ofstream file_io(qUtf8Printable(filename));
  file_io << song.json();
  file_io.close();
  current_file = filename;
  undo_stack_pointer->setClean();
}

void SongEditor::export_recording() {
  QFileDialog dialog(this);
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setDefaultSuffix(".wav");
  dialog.setDirectory(current_folder);
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setNameFilter("WAV file (*.wav)");

  dialog.setLabelText(QFileDialog::Accept, "Export");

  if (dialog.exec() != 0) {
    current_folder = dialog.directory().absolutePath();
    export_to(dialog.selectedFiles()[0].toStdString());
  }
}

void SongEditor::open() {
  if (undo_stack_pointer->isClean() ||
      QMessageBox::question(nullptr, tr("Unsaved changes"),
                            tr("Discard unsaved changes?")) ==
          QMessageBox::Yes) {
    QFileDialog dialog(this);

    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setDefaultSuffix(".json");
    dialog.setDirectory(current_folder);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter("JSON file (*.json)");

    if (dialog.exec() != 0) {
      current_folder = dialog.directory().absolutePath();
      open_file(dialog.selectedFiles()[0]);
    }
  }
}

void SongEditor::initialize_controls() {
  set_starting_control(starting_instrument_id,
                       starting_value(starting_instrument_id), true);
  set_starting_control(starting_key_id, starting_value(starting_key_id), true);
  set_starting_control(starting_volume_id, starting_value(starting_volume_id),
                       true);
  set_starting_control(starting_tempo_id, starting_value(starting_tempo_id),
                       true);
}

void SongEditor::open_file(const QString &filename) {
  try {
    std::ifstream file_io(qUtf8Printable(filename));
    auto json_song = nlohmann::json::parse(file_io);
    file_io.close();
    if (Song::verify_json(json_song)) {
      chords_model_pointer->begin_reset_model();
      song.load(json_song);
      chords_model_pointer->end_reset_model();
      initialize_controls();
      undo_stack_pointer->resetClean();
    }
  } catch (const nlohmann::json::parse_error &parse_error) {
    JsonErrorHandler::show_parse_error(parse_error.what());
    return;
  }
}

void SongEditor::paste_text(int first_child_number, const std::string &text,
                            const QModelIndex &parent_index) {
  nlohmann::json json_song;
  try {
    json_song = nlohmann::json::parse(text);
  } catch (const nlohmann::json::parse_error &parse_error) {
    JsonErrorHandler::show_parse_error(parse_error.what());
    return;
  }

  if (!ChordsModel::verify_children(parent_index, json_song)) {
    return;
  }
  undo_stack_pointer->push(
      std::make_unique<InsertRemoveChange>(
          chords_model_pointer, first_child_number, json_song,
          chords_model_pointer->get_chord_number(parent_index), true)
          .release());
}

void SongEditor::undo() { undo_stack_pointer->undo(); }

void SongEditor::redo() { undo_stack_pointer->redo(); }

auto SongEditor::get_chords_model_pointer() const -> QAbstractItemModel * {
  return chords_model_pointer;
}

auto SongEditor::get_chords_view_pointer() const -> QAbstractItemView * {
  return chords_view_pointer;
}

auto SongEditor::get_number_of_children(int chord_number) const -> int {
  const auto &chord_pointers = song.chord_pointers;
  if (chord_number == -1) {
    return static_cast<int>(chord_pointers.size());
  }
  return static_cast<int>(
      chord_pointers[static_cast<size_t>(chord_number)]->note_pointers.size());
};

SongEditor::~SongEditor() {
  undo_stack_pointer->disconnect();
  delete_fluid_audio_driver(audio_driver_pointer);
  delete_fluid_event(event_pointer);
  delete_fluid_sequencer(sequencer_pointer);
  delete_fluid_synth(synth_pointer);
  delete_fluid_settings(settings_pointer);
}

void SongEditor::play_notes(const Chord *chord_pointer, int first_note_index,
                            int number_of_notes) const {
  if (number_of_notes == -1) {
    number_of_notes = static_cast<int>(chord_pointer->note_pointers.size());
  }
  const auto &note_pointers = chord_pointer->note_pointers;
  for (auto note_index = first_note_index;
       note_index < first_note_index + number_of_notes;
       note_index = note_index + 1) {
    const auto &note_pointer = note_pointers[static_cast<size_t>(note_index)];
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

    fluid_event_program_select(
        event_pointer, note_index, static_cast<unsigned int>(soundfont_id),
        static_cast<int16_t>(instrument_pointer->bank_number),
        static_cast<int16_t>(instrument_pointer->preset_number));
    fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                            static_cast<unsigned int>(current_time), 1);

    fluid_event_pitch_bend(
        event_pointer, note_index,
        static_cast<int>((key_float - closest_key + ZERO_BEND_HALFSTEPS) *
                         BEND_PER_HALFSTEP));
    fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                            static_cast<unsigned int>(current_time), 1);

    fluid_event_noteon(
        event_pointer, note_index, static_cast<int16_t>(closest_key),
        static_cast<int16_t>(current_volume * note_pointer->volume_percent /
                             PERCENT * MAX_VELOCITY));
    fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                            static_cast<unsigned int>(current_time), 1);

    fluid_event_noteoff(event_pointer, note_index,
                        static_cast<int16_t>(closest_key));
    fluid_sequencer_send_at(
        sequencer_pointer, event_pointer,
        static_cast<unsigned int>(current_time +
                                  (beat_time() * note_pointer->beats *
                                   note_pointer->tempo_percent / PERCENT) *
                                      MILLISECONDS_PER_SECOND),
        1);
  }
}

void SongEditor::play_chords(int first_chord_index, int number_of_chords) {
  const auto &chord_pointers = song.chord_pointers;
  if (number_of_chords == -1) {
    number_of_chords = static_cast<int>(chord_pointers.size());
  }
  for (auto chord_index = first_chord_index;
       chord_index < first_chord_index + number_of_chords;
       chord_index = chord_index + 1) {
    const auto *chord_pointer =
        chord_pointers[static_cast<size_t>(chord_index)].get();
    modulate(chord_pointer);
    play_notes(chord_pointer);
    current_time = current_time + (beat_time() * chord_pointer->beats) *
                                      MILLISECONDS_PER_SECOND;
  }
}

void SongEditor::export_to(const std::string &output_file) {
  stop();
  delete_fluid_audio_driver(audio_driver_pointer);
  fluid_settings_setstr(settings_pointer, "audio.driver", "file");
  fluid_settings_setstr(settings_pointer, "audio.file.name",
                        output_file.c_str());
  play_chords();
  audio_driver_pointer =
      new_fluid_audio_driver(settings_pointer, synth_pointer);
  delete_fluid_audio_driver(audio_driver_pointer);
  start_real_time();
}
