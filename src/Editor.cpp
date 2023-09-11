#include "Editor.h"

#include <qabstractitemmodel.h>  // for QModelIndex
#include <qabstractitemview.h>   // for QAbstractItemView, QAbstractItem...
#include <qabstractslider.h>     // for QAbstractSlider
#include <qbytearray.h>          // for QByteArray
#include <qcontainerfwd.h>        // for QStringList
#include <qclipboard.h>
#include <qfile.h>        // for QFile
#include <qfiledialog.h>  // for QFileDialog
#include <qguiapplication.h>
#include <qheaderview.h>          // for QHeaderView, QHeaderView::Resize...
#include <qiodevicebase.h>        // for QIODeviceBase::ReadOnly, QIODevi...
#include <qitemselectionmodel.h>  // for QItemSelectionModel, QItemSelection
#include <qjsondocument.h>        // for QJsonDocument
#include <qkeysequence.h>         // for QKeySequence, QKeySequence::AddTab
#include <qlabel.h>               // for QLabel
#include <qlist.h>                // for QList, QList<>::const_iterator
#include <qmenubar.h>             // for QMenuBar
#include <qmessagebox.h>
#include <qmetatype.h>  // for QMetaType
#include <qmimedata.h>
#include <qslider.h>         // for QSlider
#include <qstandardpaths.h>  // for QStandardPaths, QStandardPaths::...
#include <qundostack.h>      // for QUndoStack

#include <memory>  // for make_unique, __unique_ptr_t, unique...
#include <vector>  // for vector

#include "ChordsModel.h"         // for ChordsModel
#include "ComboBoxDelegate.h"    // for ComboBoxDelegate, MAX_COMBO_BOX_...
#include "Interval.h"            // for Interval
#include "IntervalDelegate.h"    // for IntervalDelegate
#include "NoteChord.h"           // for NoteChord, chord_level, error_level
#include "Player.h"              // for Player
#include "ShowSlider.h"          // for ShowSlider
#include "ShowSliderDelegate.h"  // for ShowSliderDelegate
#include "Song.h"                // for Song, FULL_NOTE_VOLUME, SECONDS_...
#include "SpinBoxDelegate.h"     // for SpinBoxDelegate
#include "SuffixedNumber.h"      // for SuffixedNumber
#include "TreeNode.h"            // for TreeNode
#include "commands.h"            // for Insert, InsertEmptyRows, Remove
#include "utilities.h"           // for error_empty, set_combo_box, cann...

Editor::Editor(Song &song_input, QWidget *parent_pointer, Qt::WindowFlags flags)
    : song(song_input),
      clipboard_pointer(QGuiApplication::clipboard()),
      QMainWindow(parent_pointer, flags) {

  QMetaType::registerConverter<Interval, QString>(&Interval::get_text);
  QMetaType::registerConverter<SuffixedNumber, QString>(
      &SuffixedNumber::get_text);

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

  connect(export_as_action_pointer, &QAction::triggered, this,
          &Editor::export_recording);
  file_menu_pointer->addAction(export_as_action_pointer);

  menuBar()->addMenu(file_menu_pointer);

  undo_action_pointer->setShortcuts(QKeySequence::Undo);
  undo_action_pointer->setEnabled(false);
  connect(undo_action_pointer, &QAction::triggered, &undo_stack,
          &QUndoStack::undo);
  edit_menu_pointer->addAction(undo_action_pointer);

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

  // TODO: factor first/before/after?
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

  menuBar()->addMenu(edit_menu_pointer);

  view_controls_checkbox_pointer->setCheckable(true);
  view_controls_checkbox_pointer->setChecked(true);
  connect(view_controls_checkbox_pointer, &QAction::toggled, this,
          &Editor::view_controls);
  view_menu_pointer->addAction(view_controls_checkbox_pointer);

  menuBar()->addMenu(view_menu_pointer);

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

  menuBar()->addMenu(play_menu_pointer);

  starting_key_show_slider_pointer->slider_pointer->setValue(
      static_cast<int>(song.starting_key));
  connect(starting_key_show_slider_pointer->slider_pointer,
          &QAbstractSlider::valueChanged, this, &Editor::set_starting_key);
  controls_form_pointer->addRow(starting_key_label_pointer,
                                starting_key_show_slider_pointer);

  starting_volume_show_slider_pointer->slider_pointer->setValue(
      static_cast<int>(song.starting_volume));
  connect(starting_volume_show_slider_pointer->slider_pointer,
          &QAbstractSlider::valueChanged, this, &Editor::set_starting_volume);
  controls_form_pointer->addRow(starting_volume_label_pointer,
                                starting_volume_show_slider_pointer);

  starting_tempo_show_slider_pointer->slider_pointer->setValue(
      static_cast<int>(song.starting_tempo));
  connect(starting_tempo_show_slider_pointer->slider_pointer,
          &QAbstractSlider::valueChanged, this, &Editor::set_starting_tempo);
  controls_form_pointer->addRow(starting_tempo_label_pointer,
                                starting_tempo_show_slider_pointer);

  starting_instrument_selector_pointer->setModel(instruments_model_pointer);
  starting_instrument_selector_pointer->setMaxVisibleItems(MAX_COMBO_BOX_ITEMS);
  starting_instrument_selector_pointer->setStyleSheet("combobox-popup: 0;");
  starting_instrument_selector_pointer->setCurrentText(song.starting_instrument);
  connect(starting_instrument_selector_pointer, &QComboBox::currentIndexChanged,
          this, &Editor::save_starting_instrument);
  controls_form_pointer->addRow(starting_instrument_label_pointer,
                                starting_instrument_selector_pointer);

  controls_pointer->setLayout(controls_form_pointer);

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

  central_layout_pointer->addWidget(chords_view_pointer);

  central_widget_pointer->setLayout(central_layout_pointer);

  controls_pointer->setFixedWidth(CONTROLS_WIDTH);

  resize(STARTING_WINDOW_WIDTH, STARTING_WINDOW_HEIGHT);

  setWindowTitle("Justly");
  setCentralWidget(central_widget_pointer);
}

void Editor::copy_selected() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    return;
  }
  auto first_index = chords_selection[0];
  auto parent_index = chords_model_pointer->parent(first_index);
  copy_level = chords_model_pointer->get_level(parent_index) + 1;
  auto json_array = chords_model_pointer->copy_json(
      first_index.row(), static_cast<int>(chords_selection.size()),
      parent_index);
  auto new_data_pointer = std::make_unique<QMimeData>();
  new_data_pointer->setData("application/json",
                            QJsonDocument(json_array).toJson());
  clipboard_pointer->setMimeData(new_data_pointer.release());
  update_selection_and_actions();
}

void Editor::play_selected() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    return;
  }
  auto first_index = chords_selection[0];
  play(first_index.row(), static_cast<int>(chords_selection.size()),
       chords_model_pointer->parent(first_index));
}

void Editor::save_starting_instrument(int new_index) {
  auto new_starting_instrument = song.instruments[new_index].name;
  if (new_starting_instrument != song.starting_instrument) {
    undo_stack.push(std::make_unique<StartingInstrumentChange>(
                        *this, new_starting_instrument)
                        .release());
  }
}

void Editor::set_starting_instrument(const QString &new_starting_instrument,
                                     bool should_set_box) {
  song.starting_instrument = new_starting_instrument;
  if (should_set_box) {
    starting_instrument_selector_pointer->blockSignals(true);
    starting_instrument_selector_pointer->setCurrentText(new_starting_instrument);
    starting_instrument_selector_pointer->blockSignals(false);
  }
}

void Editor::insert_before() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &first_index = chords_selection[0];
  insert(first_index.row(), 1, first_index.parent());
};

void Editor::insert_after() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &last_index = chords_selection[chords_selection.size() - 1];
  insert(last_index.row() + 1, 1, last_index.parent());
};

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

void Editor::view_controls() {
  controls_pointer->setVisible(view_controls_checkbox_pointer->isChecked());
}

void Editor::remove_selected() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &first_index = chords_selection[0];
  undo_stack.push(std::make_unique<Remove>(*this, first_index.row(),
                                           chords_selection.size(),
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
  auto no_chords = song.root.number_of_children() == 0;
  auto chords_selection = selection_model_pointer->selectedRows();
  auto any_selected = !(chords_selection.isEmpty());
  auto selected_level = 0;
  auto empty_chord_is_selected = false;
  auto level_match = false;
  if (any_selected) {
    const auto &first_node =
        chords_model_pointer->const_node_from_index(chords_selection[0]);
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
};

auto Editor::set_starting_key() -> void {
  if (song.starting_key !=
      starting_key_show_slider_pointer->slider_pointer->value()) {
    undo_stack.push(
        std::make_unique<StartingKeyChange>(
            *this, starting_key_show_slider_pointer->slider_pointer->value())
            .release());
  }
}

auto Editor::set_starting_volume() -> void {
  if (song.starting_volume !=
      starting_volume_show_slider_pointer->slider_pointer->value()) {
    undo_stack.push(
        std::make_unique<StartingVolumeChange>(
            *this, starting_volume_show_slider_pointer->slider_pointer->value())
            .release());
  }
}

void Editor::set_starting_tempo() {
  if (song.starting_tempo !=
      starting_tempo_show_slider_pointer->slider_pointer->value()) {
    undo_stack.push(
        std::make_unique<StartingTempoChange>(
            *this, starting_tempo_show_slider_pointer->slider_pointer->value())
            .release());
  }
}

void Editor::insert(int first_index, int number_of_children,
                    const QModelIndex &parent_index) {
  // insertRows will error if invalid
  undo_stack.push(std::make_unique<InsertEmptyRows>(
                      *this, first_index, number_of_children, parent_index)
                      .release());
};

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
    output.write(song.to_json().toJson());
    output.close();
    unsaved_changes = false;
  } else {
    cannot_open_error(current_file);
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
    output.write(song.to_json().toJson());
    output.close();
  } else {
    cannot_open_error(filename);
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
  player_pointer = std::make_unique<Player>(song, filename);
  player_pointer->write_song();
  player_pointer = std::make_unique<Player>(song);
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
                            tr("Discard unsaved changes?")) == QMessageBox::Yes) {
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
    load_text(input.readAll());
    input.close();
    undo_stack.resetClean();
    change_file_to(filename);
    undo_action_pointer->setEnabled(false);
  } else {
    cannot_open_error(filename);
  }
}

void Editor::paste_text(int first_index, const QByteArray &paste_text,
                        const QModelIndex &parent_index) {
  if (!chords_model_pointer->verify_json_children(paste_text, parent_index)) {
    return;
  }
  const QJsonDocument document = QJsonDocument::fromJson(paste_text);
  const auto json_array = document.array();
  undo_stack.push(
      std::make_unique<Insert>(*this, first_index, json_array, parent_index)
          .release());
}

void Editor::load_text(const QByteArray &song_text) {
  chords_model_pointer->begin_reset_model();
  if (song.load_text(song_text)) {
    starting_instrument_selector_pointer->blockSignals(true);
    starting_instrument_selector_pointer->setCurrentText(song.starting_instrument);
    starting_instrument_selector_pointer->blockSignals(false);
    starting_key_show_slider_pointer->set_value_no_signals(
        static_cast<int>(song.starting_key));
    starting_volume_show_slider_pointer->set_value_no_signals(
        static_cast<int>(song.starting_volume));
    starting_tempo_show_slider_pointer->set_value_no_signals(
        static_cast<int>(song.starting_tempo));
  }
  chords_model_pointer->end_reset_model();
}

void Editor::play(int first_index, int number_of_children,
                  const QModelIndex &parent_index) {
  player_pointer->write_chords(first_index, number_of_children,
                chords_model_pointer->get_node(parent_index));
}

void Editor::stop_playing() const {
  player_pointer -> stop_playing();
}

void Editor::register_changed() {
  if (!undo_action_pointer->isEnabled()) {
    undo_action_pointer->setEnabled(true);
  }
  if (!unsaved_changes) {
    unsaved_changes = true;
  }
}