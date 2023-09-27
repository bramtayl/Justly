#include "main/Editor.h"

#include <qabstractitemmodel.h>    // for QModelIndex
#include <qabstractitemview.h>  // for QAbstractItemView
#include <qabstractslider.h>    // for QAbstractSlider
#include <qboxlayout.h>         // for QVBoxLayout
#include <qbytearray.h>            // for QByteArray
#include <qclipboard.h>             // for QClipboard
#include <qcombobox.h>          // for QComboBox
#include <qcontainerfwd.h>         // for QStringList
#include <qfile.h>                 // for QFile
#include <qfiledialog.h>        // for QFileDialog, QFileDia...
#include <qformlayout.h>        // for QFormLayout
#include <qheaderview.h>        // for QHeaderView, QHeaderV...
#include <qiodevicebase.h>         // for QIODeviceBase, QIODev...
#include <qitemselectionmodel.h>   // for QItemSelectionModel
#include <qkeysequence.h>           // for QKeySequence, QKeySeq...
#include <qlabel.h>             // for QLabel
#include <qlist.h>                 // for QList, QList<>::const...
#include <qmenu.h>              // for QMenu
#include <qmenubar.h>           // for QMenuBar
#include <qmessagebox.h>        // for QMessageBox, QMessage...
#include <qmetatype.h>             // for QMetaType
#include <qmimedata.h>             // for QMimeData
#include <qnamespace.h>            // for WindowFlags
#include <qslider.h>            // for QSlider
#include <qstandardpaths.h>        // for QStandardPaths, QStan...
#include <qstring.h>               // for QString, operator<
#include <qtreeview.h>          // for QTreeView
#include <qvariant.h>              // for QVariant
#include <qwidget.h>            // for QWidget

#include <initializer_list>       // for initializer_list
#include <map>                    // for operator!=, operator==
#include <memory>                 // for make_unique, __unique...
#include <nlohmann/json.hpp>      // for basic_json, basic_jso...
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>                 // for basic_string
#include <vector>                 // for vector

#include "commands/CellChange.h"                // for CellChange
#include "commands/InsertChange.h"              // for InsertChange
#include "commands/InsertNewChange.h"           // for InsertNewChange
#include "commands/RemoveChange.h"              // for RemoveChange
#include "commands/StartingInstrumentChange.h"  // for StartingInstrumentChange
#include "commands/StartingKeyChange.h"         // for StartingKeyChange
#include "commands/StartingTempoChange.h"       // for StartingTempoChange
#include "commands/StartingVolumeChange.h"      // for StartingVolumeChange
#include "delegates/InstrumentDelegate.h"       // for InstrumentDelegate
#include "delegates/IntervalDelegate.h"         // for IntervalDelegate
#include "delegates/ShowSliderDelegate.h"       // for ShowSliderDelegate
#include "delegates/SpinBoxDelegate.h"          // for SpinBoxDelegate
#include "editors/ShowSlider.h"                 // for ShowSlider
#include "main/Player.h"                        // for Player
#include "main/Song.h"                          // for Song
#include "main/TreeNode.h"                      // for TreeNode
#include "metatypes/Instrument.h"               // for Instrument
#include "metatypes/Interval.h"                 // for Interval
#include "metatypes/SuffixedNumber.h"           // for SuffixedNumber
#include "models/ChordsModel.h"                 // for ChordsModel
#include "models/InstrumentsModel.h"            // for InstrumentsModel
#include "notechord/NoteChord.h"                // for chord_level, beats_co...
#include "utilities/utilities.h"                // for show_open_error, show...

Editor::Editor(Song* song_pointer_input, QWidget *parent_pointer, Qt::WindowFlags flags)
    : QMainWindow(parent_pointer, flags), song_pointer(song_pointer_input) {
  QMetaType::registerConverter<Interval, QString>(&Interval::get_text);
  QMetaType::registerConverter<SuffixedNumber, QString>(
      &SuffixedNumber::get_text);
  QMetaType::registerConverter<Instrument, QString>(&Instrument::get_text);

  auto *const menu_bar_pointer = menuBar();

  QMenu* file_menu_pointer =
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

  auto *const export_as_action_pointer =
      std::make_unique<QAction>(tr("&Export recording"), file_menu_pointer)
          .release();
  connect(export_as_action_pointer, &QAction::triggered, this,
          &Editor::export_recording);
  file_menu_pointer->addAction(export_as_action_pointer);

  menu_bar_pointer->addMenu(file_menu_pointer);

  undo_action_pointer->setShortcuts(QKeySequence::Undo);
  undo_action_pointer->setEnabled(false);
  connect(undo_action_pointer, &QAction::triggered, &undo_stack,
          &QUndoStack::undo);
  edit_menu_pointer->addAction(undo_action_pointer);

   QAction* redo_action_pointer =
      std::make_unique<QAction>(tr("&Redo"), edit_menu_pointer).release();

  redo_action_pointer->setShortcuts(QKeySequence::Redo);
  edit_menu_pointer->addAction(redo_action_pointer);
  connect(redo_action_pointer, &QAction::triggered, &undo_stack,
          &QUndoStack::redo);
  edit_menu_pointer->addSeparator();

  copy_action_pointer->setEnabled(false);
  copy_action_pointer->setShortcuts(QKeySequence::Copy);
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
  connect(insert_before_action_pointer, &QAction::triggered, this,
          &Editor::insert_before);
  insert_menu_pointer->addAction(insert_before_action_pointer);

  insert_after_action_pointer->setEnabled(false);
  insert_after_action_pointer->setShortcuts(QKeySequence::InsertLineSeparator);
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
  connect(remove_action_pointer, &QAction::triggered, this,
          &Editor::remove_selected);
  edit_menu_pointer->addAction(remove_action_pointer);

  menu_bar_pointer->addMenu(edit_menu_pointer);

  view_controls_checkbox_pointer->setCheckable(true);
  view_controls_checkbox_pointer->setChecked(true);
  connect(view_controls_checkbox_pointer, &QAction::toggled, this,
          &Editor::view_controls);
  view_menu_pointer->addAction(view_controls_checkbox_pointer);

  menu_bar_pointer->addMenu(view_menu_pointer);

  play_selection_action_pointer->setEnabled(false);
  play_selection_action_pointer->setShortcuts(QKeySequence::Print);
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

  starting_key_editor_pointer->slider_pointer->setValue(
      static_cast<int>(song_pointer->starting_key));
  connect(starting_key_editor_pointer->slider_pointer,
          &QAbstractSlider::valueChanged, this, &Editor::set_starting_key);
  controls_form_pointer->addRow(
      std::make_unique<QLabel>(tr("Starting key"), controls_pointer).release(),
      starting_key_editor_pointer);

  starting_volume_editor_pointer->slider_pointer->setValue(
      static_cast<int>(song_pointer->starting_volume));
  connect(starting_volume_editor_pointer->slider_pointer,
          &QAbstractSlider::valueChanged, this, &Editor::set_starting_volume);
  controls_form_pointer->addRow(
      std::make_unique<QLabel>(tr("Starting volume"), controls_pointer)
          .release(),
      starting_volume_editor_pointer);

  starting_tempo_editor_pointer->slider_pointer->setValue(
      static_cast<int>(song_pointer->starting_tempo));
  connect(starting_tempo_editor_pointer->slider_pointer,
          &QAbstractSlider::valueChanged, this, &Editor::set_starting_tempo);
  controls_form_pointer->addRow(
      std::make_unique<QLabel>(tr("Starting tempo"), controls_pointer)
          .release(),
      starting_tempo_editor_pointer);

  starting_instrument_editor_pointer->setModel(
      std::make_unique<InstrumentsModel>(false,
                                         starting_instrument_editor_pointer)
          .release());
  starting_instrument_editor_pointer->setMaxVisibleItems(MAX_COMBO_BOX_ITEMS);
  starting_instrument_editor_pointer->setStyleSheet("combobox-popup: 0;");
  starting_instrument_editor_pointer->set_instrument(
      song_pointer->starting_instrument);
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
  chords_view_pointer->setItemDelegateForColumn(interval_column,
                                                interval_delegate_pointer);
  chords_view_pointer->setItemDelegateForColumn(beats_column,
                                                beats_delegate_pointer);
  chords_view_pointer->setItemDelegateForColumn(
      volume_percent_column, volume_percent_delegate_pointer);
  chords_view_pointer->setItemDelegateForColumn(tempo_percent_column,
                                                tempo_percent_delegate_pointer);
  chords_view_pointer->setItemDelegateForColumn(instrument_column,
                                                instrument_delegate_pointer);
  chords_view_pointer->setSelectionMode(QAbstractItemView::ContiguousSelection);
  chords_view_pointer->setSelectionBehavior(QAbstractItemView::SelectRows);
  connect(chords_view_pointer->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &Editor::update_selection_and_actions);

  connect(chords_model_pointer, &ChordsModel::about_to_set_data, this,
          &Editor::data_set);

  central_layout_pointer->addWidget(chords_view_pointer);

  central_widget_pointer->setLayout(central_layout_pointer);

  controls_pointer->setFixedWidth(CONTROLS_WIDTH);

  resize(STARTING_WINDOW_WIDTH, STARTING_WINDOW_HEIGHT);

  setWindowTitle("Justly");
  setCentralWidget(central_widget_pointer);
}

void Editor::data_set(const QModelIndex &index, const QVariant &old_value,
                      const QVariant &new_value) {
  undo_stack.push(
      std::make_unique<CellChange>(this, index, old_value, new_value)
          .release());
}

void Editor::copy_selected() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    return;
  }
  auto first_index = chords_selection[0];
  auto parent_index = chords_model_pointer->parent(first_index);
  copy_level =
      chords_model_pointer->get_const_node(parent_index).get_level() + 1;
  auto json_array =
      chords_model_pointer->get_node(parent_index)
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
       chords_model_pointer->parent(first_index));
}

void Editor::save_starting_instrument(int new_index) {
  auto new_starting_instrument = Instrument::get_all_instruments()[new_index];
  if (!(new_starting_instrument == song_pointer->starting_instrument)) {
    undo_stack.push(std::make_unique<StartingInstrumentChange>(
                        this, new_starting_instrument)
                        .release());
  }
}

void Editor::set_starting_instrument(const Instrument &new_starting_instrument,
                                     bool should_set_box) {
  song_pointer->starting_instrument = new_starting_instrument;
  if (should_set_box) {
    starting_instrument_editor_pointer->blockSignals(true);
    starting_instrument_editor_pointer->set_instrument(
        new_starting_instrument);
    starting_instrument_editor_pointer->blockSignals(false);
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

void Editor::view_controls() const {
  controls_pointer->setVisible(view_controls_checkbox_pointer->isChecked());
}

void Editor::remove_selected() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &first_index = chords_selection[0];
  undo_stack.push(std::make_unique<RemoveChange>(this, first_index.row(),
                                                 chords_selection.size(),
                                                 first_index.parent())
                      .release());
  update_selection_and_actions();
}

void Editor::update_selection_and_actions() const {
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
  auto any_selected = !(chords_selection.isEmpty());
  auto selected_level = 0;
  auto empty_chord_is_selected = false;
  auto level_match = false;
  if (any_selected) {
    const auto &first_node =
        chords_model_pointer->get_const_node(chords_selection[0]);
    selected_level = first_node.get_level();
    level_match = selected_level == copy_level;
    empty_chord_is_selected = chords_selection.size() == 1 &&
                              selected_level == chord_level &&
                              first_node.number_of_children() == 0;
  }

  play_selection_action_pointer->setEnabled(any_selected);
  insert_before_action_pointer->setEnabled(any_selected);
  insert_after_action_pointer->setEnabled(any_selected);
  remove_action_pointer->setEnabled(any_selected);
  copy_action_pointer->setEnabled(any_selected);

  paste_before_action_pointer->setEnabled(level_match);
  paste_after_action_pointer->setEnabled(level_match);

  insert_into_action_pointer->setEnabled(no_chords || empty_chord_is_selected);
  paste_into_action_pointer->setEnabled(
      (no_chords && copy_level == chord_level) ||
      (empty_chord_is_selected && copy_level == note_level));
}

auto Editor::set_starting_key() -> void {
  if (song_pointer->starting_key !=
      starting_key_editor_pointer->slider_pointer->value()) {
    undo_stack.push(
        std::make_unique<StartingKeyChange>(
            this, starting_key_editor_pointer->slider_pointer->value())
            .release());
  }
}

auto Editor::set_starting_volume() -> void {
  if (song_pointer->starting_volume !=
      starting_volume_editor_pointer->slider_pointer->value()) {
    undo_stack.push(
        std::make_unique<StartingVolumeChange>(
            this, starting_volume_editor_pointer->slider_pointer->value())
            .release());
  }
}

void Editor::set_starting_tempo() {
  if (song_pointer->starting_tempo !=
      starting_tempo_editor_pointer->slider_pointer->value()) {
    undo_stack.push(
        std::make_unique<StartingTempoChange>(
            this, starting_tempo_editor_pointer->slider_pointer->value())
            .release());
  }
}

void Editor::insert(int first_index, int number_of_children,
                    const QModelIndex &parent_index) {
  // insertRows will error if invalid
  undo_stack.push(std::make_unique<InsertNewChange>(
                      this, first_index, number_of_children, parent_index)
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
  QFile output(current_file);
  if (output.open(QIODeviceBase::WriteOnly)) {
    output.write(song_pointer->to_json().dump().data());
    output.close();
    unsaved_changes = false;
  } else {
    show_open_error(current_file);
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
  change_file_to(filename);
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
  player_pointer = std::make_unique<Player>(song_pointer, filename);
  player_pointer->write_song();
  player_pointer = std::make_unique<Player>(song_pointer);
}

void Editor::change_file_to(const QString &filename) {
  current_file = filename;
  if (!save_action_pointer->isEnabled()) {
    save_action_pointer->setEnabled(true);
  }
  unsaved_changes = false;
}

void Editor::open() {
  if (!unsaved_changes ||
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

void Editor::open_file(const QString &filename) {
  QFile input(filename);
  if (input.open(QIODeviceBase::ReadOnly)) {
    auto song_text = input.readAll();
    chords_model_pointer->begin_reset_model();
    if (song_pointer->load_text(song_text)) {
      starting_instrument_editor_pointer->blockSignals(true);
      starting_instrument_editor_pointer->set_instrument(
          song_pointer->starting_instrument);
      starting_instrument_editor_pointer->blockSignals(false);
      starting_key_editor_pointer->set_value_no_signals(
          static_cast<int>(song_pointer->starting_key));
      starting_volume_editor_pointer->set_value_no_signals(
          static_cast<int>(song_pointer->starting_volume));
      starting_tempo_editor_pointer->set_value_no_signals(
          static_cast<int>(song_pointer->starting_tempo));
    }
    chords_model_pointer->end_reset_model();
    input.close();
    undo_stack.resetClean();
    change_file_to(filename);
    undo_action_pointer->setEnabled(false);
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

  if (!chords_model_pointer->get_const_node(parent_index)
           .verify_json_children(parsed_json)) {
    return;
  }
  undo_stack.push(std::make_unique<InsertChange>(this, first_index,
                                                 parsed_json, parent_index)
                      .release());
}

void Editor::play(int first_index, int number_of_children,
                  const QModelIndex &parent_index) const {
  player_pointer->write_chords(first_index, number_of_children,
                               chords_model_pointer->get_node(parent_index));
}

void Editor::stop_playing() const { player_pointer->stop_playing(); }

void Editor::register_changed() {
  if (!undo_action_pointer->isEnabled()) {
    undo_action_pointer->setEnabled(true);
  }
  if (!unsaved_changes) {
    unsaved_changes = true;
  }
}
