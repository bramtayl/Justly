#include "main/Editor.h"

#include <qabstractitemmodel.h>  // for QModelIndex
#include <qabstractitemview.h>   // for QAbstractItemView, QAbstra...
#include <qboxlayout.h>          // for QVBoxLayout
#include <qbytearray.h>          // for QByteArray
#include <qclipboard.h>          // for QClipboard
#include <qcombobox.h>           // for QComboBox
#include <qcontainerfwd.h>       // for QStringList
#include <qfile.h>               // for QFile
#include <qfiledialog.h>         // for QFileDialog, QFileDialog::...
#include <qformlayout.h>         // for QFormLayout
#include <qheaderview.h>         // for QHeaderView, QHeaderView::...
#include <qiodevicebase.h>       // for QIODeviceBase, QIODeviceBa...
#include <qitemeditorfactory.h>
#include <qitemselectionmodel.h>  // for QItemSelectionModel, QItem...
#include <qkeysequence.h>         // for QKeySequence, QKeySequence...
#include <qlabel.h>               // for QLabel
#include <qlineedit.h>
#include <qlist.h>        // for QList, QList<>::const_iter...
#include <qmenu.h>        // for QMenu
#include <qmenubar.h>     // for QMenuBar
#include <qmessagebox.h>  // for QMessageBox, QMessageBox::Yes
#include <qmetaobject.h>
#include <qmetatype.h>
#include <qmimedata.h>       // for QMimeData
#include <qnamespace.h>      // for WindowFlags
#include <qstandardpaths.h>  // for QStandardPaths, QStandardP...
#include <qstring.h>         // for QString
#include <qtreeview.h>       // for QTreeView
#include <qvariant.h>        // for QVariant, operator!=
#include <qwidget.h>         // for QWidget

#include <initializer_list>       // for initializer_list
#include <map>                    // for operator!=, operator==
#include <memory>                 // for make_unique, __unique_ptr_t
#include <nlohmann/json.hpp>      // for basic_json, basic_json<>::...
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>                 // for basic_string
#include <vector>                 // for vector

#include "commands/CellChange.h"           // for CellChange
#include "commands/InsertChange.h"         // for InsertChange
#include "commands/InsertNewChange.h"      // for InsertNewChange
#include "commands/RemoveChange.h"         // for RemoveChange
#include "commands/StartingValueChange.h"  // for StartingValueChange
#include "editors/IntervalEditor.h"
#include "main/Player.h"           // for Player
#include "main/Song.h"             // for Song
#include "main/TreeNode.h"         // for TreeNode
#include "metatypes/Instrument.h"  // for Instrument
#include "metatypes/Interval.h"
#include "models/ChordsModel.h"   // for ChordsModel
#include "notechord/NoteChord.h"  // for chord_level, note_level
#include "utilities/utilities.h"  // for show_open_error, show_pars...

const auto STARTING_WINDOW_WIDTH = 800;
const auto STARTING_WINDOW_HEIGHT = 600;
const auto CONTROLS_WIDTH = 500;

Editor::Editor(QWidget *parent_pointer, Qt::WindowFlags flags)
    : QMainWindow(parent_pointer, flags) {
  auto *factory = std::make_unique<QItemEditorFactory>().release();

  factory->registerEditor(
      QMetaType::fromType<const Instrument *>().id(),
      std::make_unique<QStandardItemEditorCreator<InstrumentEditor>>()
          .release());
  factory->registerEditor(
      QMetaType::fromType<Interval>().id(),
      std::make_unique<QStandardItemEditorCreator<IntervalEditor>>().release());
  factory->registerEditor(
      QMetaType::Int,
      std::make_unique<QStandardItemEditorCreator<QSpinBox>>().release());
  factory->registerEditor(
      QMetaType::Double,
      std::make_unique<QStandardItemEditorCreator<QDoubleSpinBox>>().release());
  factory->registerEditor(
      QMetaType::QString,
      std::make_unique<QStandardItemEditorCreator<QLineEdit>>().release());

  QItemEditorFactory::setDefaultFactory(factory);

  auto *const menu_bar_pointer = menuBar();

  gsl::not_null<QMenu *> file_menu_pointer =
      std::make_unique<QMenu>(tr("&File"), this).release();

  auto *const open_action_pointer =
      std::make_unique<QAction>(tr("&Open"), file_menu_pointer).release();
  file_menu_pointer->addAction(open_action_pointer);
  connect(open_action_pointer, &QAction::triggered, this, &Editor::open);
  open_action_pointer->setShortcuts(QKeySequence::Open);

  file_menu_pointer->addSeparator();

  save_action_pointer->setShortcuts(QKeySequence::Save);
  connect(save_action_pointer, &QAction::triggered, this, &Editor::save);
  
  file_menu_pointer->addAction(save_action_pointer);
  save_action_pointer->setEnabled(false);

  save_as_action_pointer->setShortcuts(QKeySequence::SaveAs);
  connect(save_as_action_pointer, &QAction::triggered, this, &Editor::save_as);
  file_menu_pointer->addAction(save_as_action_pointer);
  save_as_action_pointer->setEnabled(true);

  connect(undo_stack_pointer, &QUndoStack::cleanChanged, this, &Editor::update_clean);

  auto *const export_as_action_pointer =
      std::make_unique<QAction>(tr("&Export recording"), file_menu_pointer)
          .release();
  connect(export_as_action_pointer, &QAction::triggered, this,
          &Editor::export_recording);
  file_menu_pointer->addAction(export_as_action_pointer);

  menu_bar_pointer->addMenu(file_menu_pointer);

  auto* undo_action_pointer = undo_stack_pointer->createUndoAction(edit_menu_pointer);
  undo_action_pointer->setShortcuts(QKeySequence::Undo);
  edit_menu_pointer->addAction(undo_action_pointer);

  auto* redo_action_pointer = undo_stack_pointer->createRedoAction(edit_menu_pointer);
  redo_action_pointer->setShortcuts(QKeySequence::Redo);
  edit_menu_pointer->addAction(redo_action_pointer);

  edit_menu_pointer->addSeparator();

  copy_action_pointer->setEnabled(false);
  copy_action_pointer->setShortcuts(QKeySequence::Copy);
  connect(this, &Editor::anySelectedChanged, copy_action_pointer,
          &QAction::setEnabled);
  connect(copy_action_pointer, &QAction::triggered, this,
          &Editor::copy_selected);
  edit_menu_pointer->addAction(copy_action_pointer);
  paste_before_action_pointer->setEnabled(false);

  connect(paste_before_action_pointer, &QAction::triggered, this,
          &Editor::paste_before);
  paste_menu_pointer->addAction(paste_before_action_pointer);

  paste_after_action_pointer->setEnabled(false);
  paste_after_action_pointer->setShortcuts(QKeySequence::Paste);
  connect(paste_after_action_pointer, &QAction::triggered, this,
          &Editor::paste_after);
  paste_menu_pointer->addAction(paste_after_action_pointer);

  paste_into_action_pointer->setEnabled(false);
  connect(paste_into_action_pointer, &QAction::triggered, this,
          &Editor::paste_into);
  paste_menu_pointer->addAction(paste_into_action_pointer);

  edit_menu_pointer->addMenu(paste_menu_pointer);

  edit_menu_pointer->addSeparator();

  edit_menu_pointer->addMenu(insert_menu_pointer);

  insert_before_action_pointer->setEnabled(false);
  connect(this, &Editor::anySelectedChanged, insert_before_action_pointer,
          &QAction::setEnabled);
  connect(insert_before_action_pointer, &QAction::triggered, this,
          &Editor::insert_before);
  insert_menu_pointer->addAction(insert_before_action_pointer);

  insert_after_action_pointer->setEnabled(false);
  insert_after_action_pointer->setShortcuts(QKeySequence::InsertLineSeparator);
  connect(this, &Editor::anySelectedChanged, insert_after_action_pointer,
          &QAction::setEnabled);
  connect(insert_after_action_pointer, &QAction::triggered, this,
          &Editor::insert_after);
  insert_menu_pointer->addAction(insert_after_action_pointer);

  insert_into_action_pointer->setEnabled(true);
  insert_into_action_pointer->setShortcuts(QKeySequence::AddTab);
  connect(insert_into_action_pointer, &QAction::triggered, this,
          &Editor::insert_into);
  insert_menu_pointer->addAction(insert_into_action_pointer);

  remove_action_pointer->setEnabled(false);
  remove_action_pointer->setShortcuts(QKeySequence::Delete);
  connect(this, &Editor::anySelectedChanged, remove_action_pointer,
          &QAction::setEnabled);
  connect(remove_action_pointer, &QAction::triggered, this,
          &Editor::remove_selected);
  edit_menu_pointer->addAction(remove_action_pointer);

  menu_bar_pointer->addMenu(edit_menu_pointer);

  view_controls_checkbox_pointer->setCheckable(true);
  connect(view_controls_checkbox_pointer, &QAction::toggled, this,
          &Editor::view_controls);
  view_controls_checkbox_pointer->setChecked(true);
  view_menu_pointer->addAction(view_controls_checkbox_pointer);

  menu_bar_pointer->addMenu(view_menu_pointer);

  play_selection_action_pointer->setEnabled(false);
  play_selection_action_pointer->setShortcuts(QKeySequence::Print);
  connect(this, &Editor::anySelectedChanged, play_selection_action_pointer,
          &QAction::setEnabled);
  connect(play_selection_action_pointer, &QAction::triggered, this,
          &Editor::play_selected);
  play_menu_pointer->addAction(play_selection_action_pointer);

  stop_playing_action_pointer->setEnabled(true);
  play_menu_pointer->addAction(stop_playing_action_pointer);
  connect(stop_playing_action_pointer, &QAction::triggered, this,
          &Editor::stop_playing);
  stop_playing_action_pointer->setShortcuts(QKeySequence::Cancel);

  menu_bar_pointer->addMenu(play_menu_pointer);

  auto *const controls_form_pointer =
      std::make_unique<QFormLayout>(controls_pointer).release();

  starting_key_editor_pointer->setMinimum(MINIMUM_STARTING_KEY);
  starting_key_editor_pointer->setMaximum(MAXIMUM_STARTING_KEY);
  starting_key_editor_pointer->setDecimals(1);
  starting_key_editor_pointer->setSuffix(" hz");
  connect(starting_key_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          &Editor::save_starting_key);
  controls_form_pointer->addRow(
      std::make_unique<QLabel>(tr("Starting key"), controls_pointer).release(),
      starting_key_editor_pointer);

  starting_volume_editor_pointer->setMinimum(MINIMUM_STARTING_VOLUME);
  starting_volume_editor_pointer->setMaximum(MAXIMUM_STARTING_VOLUME);
  starting_volume_editor_pointer->setDecimals(1);
  starting_volume_editor_pointer->setSuffix("%");
  connect(starting_volume_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          &Editor::save_starting_volume);
  controls_form_pointer->addRow(
      std::make_unique<QLabel>(tr("Starting volume"), controls_pointer)
          .release(),
      starting_volume_editor_pointer);

  starting_tempo_editor_pointer->setMinimum(MINIMUM_STARTING_TEMPO);
  starting_tempo_editor_pointer->setMaximum(MAXIMUM_STARTING_TEMPO);
  starting_tempo_editor_pointer->setDecimals(1);
  starting_tempo_editor_pointer->setSuffix(" bpm");
  connect(starting_tempo_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          &Editor::save_starting_tempo);
  controls_form_pointer->addRow(
      std::make_unique<QLabel>(tr("Starting tempo"), controls_pointer)
          .release(),
      starting_tempo_editor_pointer);

  initialize_controls();

  connect(starting_instrument_editor_pointer, &QComboBox::currentIndexChanged,
          this, &Editor::save_starting_instrument);
  controls_form_pointer->addRow(
      std::make_unique<QLabel>(tr("Starting instrument"), controls_pointer)
          .release(),
      starting_instrument_editor_pointer);

  controls_pointer->setLayout(controls_form_pointer);

  auto *const central_layout_pointer =
      std::make_unique<QVBoxLayout>(central_widget_pointer).release();

  central_layout_pointer->addWidget(controls_pointer);

  chords_view_pointer->header()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  chords_view_pointer->setModel(chords_model_pointer);
  chords_view_pointer->setItemDelegate(my_delegate_pointer);
  chords_view_pointer->setSelectionMode(QAbstractItemView::ContiguousSelection);
  chords_view_pointer->setSelectionBehavior(QAbstractItemView::SelectRows);
  connect(chords_view_pointer->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &Editor::update_selection_and_actions);

  connect(chords_model_pointer, &ChordsModel::should_change_cell, this,
          &Editor::change_cell);

  central_layout_pointer->addWidget(chords_view_pointer);

  central_widget_pointer->setLayout(central_layout_pointer);

  controls_pointer->setFixedWidth(CONTROLS_WIDTH);

  resize(STARTING_WINDOW_WIDTH, STARTING_WINDOW_HEIGHT);

  setWindowTitle("Justly");
  setCentralWidget(central_widget_pointer);
}

void Editor::change_cell(const QModelIndex &index, const QVariant &old_value,
                         const QVariant &new_value) {
  undo_stack_pointer->push(
      std::make_unique<CellChange>(chords_model_pointer, index, old_value, new_value)
          .release());
}

void Editor::update_clean(bool clean) {
  save_action_pointer -> setEnabled(!(clean || current_file.isEmpty()));
  save_as_action_pointer -> setEnabled(!clean);
}

void Editor::copy_selected() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    return;
  }
  auto first_index = chords_selection[0];
  auto parent_index = get_chords_model().parent(first_index);
  copy_level = get_chords_model().get_const_node(parent_index).get_level() + 1;
  auto json_array =
      get_chords_model()
          .get_node(parent_index)
          .copy_json_children(first_index.row(),
                              static_cast<int>(chords_selection.size()));
  auto *const new_data_pointer = std::make_unique<QMimeData>().release();
  new_data_pointer->setData("application/json",
                            QString::fromStdString(json_array.dump()).toUtf8());
  clipboard_pointer->setMimeData(new_data_pointer);
  update_selection_and_actions();
}

void Editor::play_selected() const {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    return;
  }
  auto first_index = chords_selection[0];
  play(first_index.row(), static_cast<int>(chords_selection.size()),
       get_chords_model().parent(first_index));
}

void Editor::save_new_starting_value(StartingFieldId value_type,
                                     const QVariant &new_value) {
  const auto &old_value = song_pointer->get_starting_value(value_type);
  if (old_value != new_value) {
    undo_stack_pointer->push(std::make_unique<StartingValueChange>(
                                 this, value_type, old_value, new_value)
                                 .release());
  }
}

void Editor::insert_before() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &first_index = chords_selection[0];
  insert(first_index.row(), 1, first_index.parent());
}

void Editor::insert_after() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &last_index = chords_selection[chords_selection.size() - 1];
  insert(last_index.row() + 1, 1, last_index.parent());
}

void Editor::insert_into() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  insert(0, 1, chords_selection.empty() ? QModelIndex() : chords_selection[0]);
}

void Editor::paste_before() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &first_index = chords_selection[0];
  paste(first_index.row(), first_index.parent());
}

void Editor::paste_after() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &last_index = chords_selection[chords_selection.size() - 1];
  paste(last_index.row() + 1, last_index.parent());
}

void Editor::paste_into() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  paste(0, chords_selection.empty() ? QModelIndex() : chords_selection[0]);
}

void Editor::view_controls(bool checked) const {
  controls_pointer->setVisible(checked);
}

void Editor::remove_selected() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &first_index = chords_selection[0];
  undo_stack_pointer->push(std::make_unique<RemoveChange>(
                               chords_model_pointer, first_index.row(), chords_selection.size(),
                               first_index.parent())
                               .release());
  update_selection_and_actions();
}

void Editor::update_selection_and_actions() {
  auto *selection_model_pointer = chords_view_pointer->selectionModel();

  const auto selection = selection_model_pointer->selectedRows();
  const auto current_parent_index =
      chords_view_pointer->currentIndex().parent();

  QItemSelection invalid;

  for (const QModelIndex &index : selection) {
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

  // revise this later
  auto no_chords = song_pointer->root.number_of_children() == 0;

  auto chords_selection = selection_model_pointer->selectedRows();
  auto new_any_selected = !(chords_selection.isEmpty());
  if (any_selected != new_any_selected) {
    emit anySelectedChanged(new_any_selected);
    any_selected = new_any_selected;
  }
  auto selected_level = root_level;
  auto empty_chord_is_selected = false;
  auto level_match = false;
  if (new_any_selected) {
    const auto &first_node =
        get_chords_model().get_const_node(chords_selection[0]);
    selected_level = first_node.get_level();
    level_match = selected_level == copy_level;
    empty_chord_is_selected = chords_selection.size() == 1 &&
                              selected_level == chord_level &&
                              first_node.number_of_children() == 0;
  }

  paste_before_action_pointer->setEnabled(level_match);
  paste_after_action_pointer->setEnabled(level_match);

  insert_into_action_pointer->setEnabled(no_chords || empty_chord_is_selected);
  paste_into_action_pointer->setEnabled(
      (no_chords && copy_level == chord_level) ||
      (empty_chord_is_selected && copy_level == note_level));
}

auto Editor::save_starting_key(int new_value) -> void {
  save_new_starting_value(starting_key_id, QVariant::fromValue(new_value));
}

auto Editor::save_starting_volume(int new_value) -> void {
  save_new_starting_value(starting_volume_id, QVariant::fromValue(new_value));
}

void Editor::save_starting_tempo(int new_value) {
  save_new_starting_value(starting_tempo_id, QVariant::fromValue(new_value));
}

void Editor::save_starting_instrument(int new_index) {
  save_new_starting_value(
      starting_instrument_id,
      QVariant::fromValue(&(Instrument::get_all_instruments().at(new_index))));
}

void Editor::insert(int first_index, int number_of_children,
                    const QModelIndex &parent_index) {
  // insertRows will error if invalid
  undo_stack_pointer->push(std::make_unique<InsertNewChange>(chords_model_pointer, first_index,
                                                             number_of_children,
                                                             parent_index)
                               .release());
}

void Editor::paste(int first_index, const QModelIndex &parent_index) {
  const QMimeData *mime_data_pointer = clipboard_pointer->mimeData();
  if (mime_data_pointer->hasFormat("application/json")) {
    paste_text(first_index, mime_data_pointer->data("application/json"),
               parent_index);
  }
}

void Editor::save() {
  QFile output(get_current_file());
  if (output.open(QIODeviceBase::WriteOnly)) {
    output.write(song_pointer->to_json().dump().data());
    output.close();
    undo_stack_pointer->setClean();
  } else {
    show_open_error(get_current_file());
  }
}

void Editor::save_as() {
  QFileDialog dialog(this);

  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setDefaultSuffix(".json");
  dialog.setDirectory(
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setNameFilter("JSON file (*.json)");

  if (dialog.exec() != 0) {
    save_as_file(dialog.selectedFiles()[0]);
  }
}

void Editor::save_as_file(const QString &filename) {
  QFile output(filename);
  if (output.open(QIODeviceBase::WriteOnly)) {
    output.write(song_pointer->to_json().dump().data());
    output.close();
  } else {
    show_open_error(filename);
    return;
  }
  current_file = filename;
}

void Editor::export_recording() {
  QFileDialog dialog(this);
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setDefaultSuffix(".wav");
  dialog.setDirectory(
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setNameFilter("WAV file (*.wav)");

  dialog.setLabelText(QFileDialog::Accept, "Export");

  if (dialog.exec() != 0) {
    export_recording_file(dialog.selectedFiles()[0]);
  }
}

void Editor::export_recording_file(const QString &filename) {
  player_pointer = std::make_unique<Player>(song_pointer.get(), filename);
  player_pointer->write_song();
  player_pointer = std::make_unique<Player>(song_pointer.get());
}

void Editor::open() {
  if (undo_stack_pointer -> isClean() ||
      QMessageBox::question(nullptr, tr("Unsaved changes"),
                            tr("Discard unsaved changes?")) ==
          QMessageBox::Yes) {
    QFileDialog dialog(this);

    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setDefaultSuffix(".json");
    dialog.setDirectory(
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter("JSON file (*.json)");

    if (dialog.exec() != 0) {
      open_file(dialog.selectedFiles()[0]);
    }
  }
}

void Editor::initialize_controls() const {
  initialize_starting_control_value(starting_instrument_id);
  initialize_starting_control_value(starting_key_id);
  initialize_starting_control_value(starting_volume_id);
  initialize_starting_control_value(starting_tempo_id);
}

void Editor::load_text(const QString &song_text) {
  nlohmann::json parsed_json;
  try {
    parsed_json = nlohmann::json::parse(song_text.toStdString());
  } catch (const nlohmann::json::parse_error &parse_error) {
    show_parse_error(parse_error);
    return;
  }

  if (Song::verify_json(parsed_json)) {
    song_pointer->load_controls(parsed_json);
    initialize_controls();
    get_chords_model().load_from(parsed_json);

    undo_stack_pointer->resetClean();
  }
}

void Editor::open_file(const QString &filename) {
  QFile input(filename);
  if (input.open(QIODeviceBase::ReadOnly)) {
    load_text(input.readAll());
    current_file = filename;
    input.close();
  } else {
    show_open_error(filename);
  }
}

void Editor::paste_text(int first_index, const QByteArray &paste_text,
                        const QModelIndex &parent_index) {
  nlohmann::json parsed_json;
  try {
    parsed_json = nlohmann::json::parse(paste_text.toStdString());
  } catch (const nlohmann::json::parse_error &parse_error) {
    show_parse_error(parse_error);
    return;
  }

  if (!get_chords_model()
           .get_const_node(parent_index)
           .verify_json_children(parsed_json)) {
    return;
  }
  undo_stack_pointer->push(std::make_unique<InsertChange>(
                               chords_model_pointer, first_index, parsed_json, parent_index)
                               .release());
}

void Editor::play(int first_index, int number_of_children,
                  const QModelIndex &parent_index) const {
  player_pointer->write_chords(first_index, number_of_children,
                               get_chords_model().get_node(parent_index));
}

void Editor::stop_playing() const { player_pointer->stop_playing(); }

auto Editor::are_controls_visible() const -> bool {
  return controls_pointer->isVisible();
}

void Editor::set_controls_visible(bool is_visible) const {
  view_controls_checkbox_pointer->setChecked(is_visible);
}

auto Editor::has_real_time() const -> bool {
  return player_pointer->has_real_time();
}

void Editor::undo() { undo_stack_pointer->undo(); }

void Editor::redo() { undo_stack_pointer->redo(); }

auto Editor::get_current_file() const -> const QString & {
  return current_file;
}

void Editor::set_current_file(const QString& new_file) {
  current_file = new_file;
}

auto Editor::get_starting_control_value(StartingFieldId value_type) const
    -> QVariant {
  if (value_type == starting_key_id) {
    return QVariant::fromValue(starting_key_editor_pointer->value());
  }
  if (value_type == starting_volume_id) {
    return QVariant::fromValue(starting_volume_editor_pointer->value());
  }
  if (value_type == starting_tempo_id) {
    return QVariant::fromValue(starting_tempo_editor_pointer->value());
  }
  return QVariant::fromValue(starting_instrument_editor_pointer->value());
}

void Editor::set_starting_control_value(StartingFieldId value_type,
                                        const QVariant &new_value) const {
  if (value_type == starting_key_id) {
    starting_key_editor_pointer->setValue(new_value.toDouble());
  } else if (value_type == starting_volume_id) {
    starting_volume_editor_pointer->setValue(new_value.toDouble());
  } else if (value_type == starting_tempo_id) {
    starting_tempo_editor_pointer->setValue(new_value.toDouble());
  } else {
    starting_instrument_editor_pointer->setValue(
        new_value.value<const Instrument *>());
  }
}

void Editor::set_starting_control_value_no_signals(
    StartingFieldId value_type, const QVariant &new_value) const {
  if (value_type == starting_key_id) {
    starting_key_editor_pointer->blockSignals(true);
    starting_key_editor_pointer->setValue(new_value.toDouble());
    starting_key_editor_pointer->blockSignals(false);
  } else if (value_type == starting_volume_id) {
    starting_volume_editor_pointer->blockSignals(true);
    starting_volume_editor_pointer->setValue(new_value.toDouble());
    starting_volume_editor_pointer->blockSignals(false);
  } else if (value_type == starting_tempo_id) {
    starting_tempo_editor_pointer->blockSignals(true);
    starting_tempo_editor_pointer->setValue(new_value.toDouble());
    starting_tempo_editor_pointer->blockSignals(false);
  } else {
    starting_instrument_editor_pointer->set_value_no_signals(
        new_value.value<const Instrument *>());
  }
  song_pointer->set_starting_value(value_type, new_value);
}

void Editor::initialize_starting_control_value(
    StartingFieldId value_type) const {
  auto result = song_pointer->get_starting_value(value_type);
  set_starting_control_value_no_signals(
      value_type, song_pointer->get_starting_value(value_type));
}

auto Editor::get_selection_model() -> QItemSelectionModel & {
  return *chords_view_pointer->selectionModel();
}

auto Editor::get_chords_model() const -> ChordsModel & {
  return *chords_model_pointer;
}

auto Editor::get_song() const -> const Song & { return *song_pointer; }

auto Editor::get_delegate() const -> const MyDelegate & {
  return *my_delegate_pointer;
}

auto Editor::get_chords_viewport_pointer() const -> QWidget * {
  return chords_view_pointer->viewport();
}