#include "Editor.h"

#include <QtCore/qglobal.h>       // for QForeachContainer, qMakeForeachCon...
#include <qabstractbutton.h>      // for QAbstractButton
#include <qabstractitemview.h>    // for QAbstractItemView, QAbstractItemVi...
#include <qabstractslider.h>      // for QAbstractSlider
#include <qbytearray.h>           // for QByteArray
#include <qfile.h>                // for QFile
#include <qfiledialog.h>          // for QFileDialog
#include <qheaderview.h>          // for QHeaderView, QHeaderView::ResizeTo...
#include <qiodevice.h>            // for QIODevice
#include <qiodevicebase.h>        // for QIODeviceBase::ReadOnly, QIODevice...
#include <qitemselectionmodel.h>  // for QItemSelectionModel, QItemSelection
#include <qjsondocument.h>        // for QJsonDocument
#include <qkeysequence.h>         // for QKeySequence, QKeySequence::AddTab
#include <qlabel.h>               // for QLabel
#include <qlist.h>                // for QList<>::const_iterator
#include <qmenubar.h>             // for QMenuBar
#include <qmessagebox.h>          // for QMessageBox
#include <qsize.h>                // for QSize
#include <qslider.h>              // for QSlider
#include <qstandardpaths.h>       // for QStandardPaths, QStandardPaths::Do...
#include <qundostack.h>           // for QUndoStack

#include <algorithm>  // for max

#include "ComboBoxItemDelegate.h"  // for ComboBoxItemDelegate
#include "NoteChord.h"             // for beats_column, denominator_column
#include "TreeNode.h"              // for TreeNode
#include "Utilities.h"             // for error_empty, set_combo_box, fill_c...
#include "commands.h"              // for OrchestraChange, DefaultInstrument...

Editor::Editor(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags),
      instrument_delegate_pointer(
          new ComboBoxItemDelegate(song_pointer->instrument_pointers)) {
  menuBar()->addMenu(file_menu_pointer);
  menuBar()->addMenu(edit_menu_pointer);
  menuBar()->addMenu(view_menu_pointer);
  menuBar()->addMenu(play_menu_pointer);

  central_box_pointer->setLayout(central_column_pointer);
  orchestra_box_pointer->setLayout(orchestra_column_pointer);

  save_orchestra_button_pointer->setFixedWidth(
      save_orchestra_button_pointer->sizeHint().width());

  controls_box_pointer->setLayout(controls_form_pointer);

  starting_key_slider_pointer->slider_pointer->setValue(
      static_cast<int>(song_pointer->starting_key));
  connect(starting_key_slider_pointer->slider_pointer,
          &QAbstractSlider::valueChanged, this,
          &Editor::set_starting_key_with_slider);
  controls_form_pointer->addRow(starting_key_label_pointer,
                                starting_key_slider_pointer);

  starting_volume_slider_pointer->slider_pointer->setValue(
      static_cast<int>(song_pointer->starting_volume));
  connect(starting_volume_slider_pointer->slider_pointer,
          &QAbstractSlider::valueChanged, this,
          &Editor::set_starting_volume_with_slider);
  controls_form_pointer->addRow(starting_volume_label_pointer,
                                starting_volume_slider_pointer);

  starting_tempo_slider_pointer->slider_pointer->setValue(
      static_cast<int>(song_pointer->starting_tempo));
  connect(starting_tempo_slider_pointer->slider_pointer,
          &QAbstractSlider::valueChanged, this,
          &Editor::set_starting_tempo_with_slider);
  controls_form_pointer->addRow(starting_tempo_label_pointer,
                                starting_tempo_slider_pointer);

  fill_combo_box(*default_instrument_selector_pointer,
                 song_pointer->instrument_pointers);
  set_combo_box(*default_instrument_selector_pointer,
                song_pointer->default_instrument);
  connect(default_instrument_selector_pointer, &QComboBox::activated, this,
          &Editor::save_default_instrument);
  controls_form_pointer->addRow(default_instrument_label_pointer,
                                default_instrument_selector_pointer);

  central_column_pointer->addWidget(controls_box_pointer);

  tree_view_pointer->setModel(song_pointer);
  tree_view_pointer->setSelectionMode(QAbstractItemView::ContiguousSelection);
  tree_view_pointer->setSelectionBehavior(QAbstractItemView::SelectRows);
  connect(tree_view_pointer->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &Editor::reenable_actions);
  tree_view_pointer->header()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  tree_view_pointer->setItemDelegateForColumn(numerator_column,
                                              numerator_delegate_pointer);
  tree_view_pointer->setItemDelegateForColumn(denominator_column,
                                              denominator_delegate_pointer);
  tree_view_pointer->setItemDelegateForColumn(octave_column,
                                              octave_delegate_pointer);
  tree_view_pointer->setItemDelegateForColumn(beats_column,
                                              beats_delegate_pointer);
  tree_view_pointer->setItemDelegateForColumn(volume_percent_column,
                                              volume_delegate_pointer);
  tree_view_pointer->setItemDelegateForColumn(tempo_percent_column,
                                              tempo_delegate_pointer);
  tree_view_pointer->setItemDelegateForColumn(instrument_column,
                                              instrument_delegate_pointer);

  file_menu_pointer->addAction(open_action_pointer);
  connect(open_action_pointer, &QAction::triggered, this, &Editor::open);
  open_action_pointer->setShortcuts(QKeySequence::Open);

  file_menu_pointer->addAction(save_action_pointer);
  connect(save_action_pointer, &QAction::triggered, this, &Editor::save);
  save_action_pointer->setShortcuts(QKeySequence::Save);

  view_menu_pointer->addAction(view_controls_action_pointer);
  view_controls_action_pointer->setCheckable(true);
  view_controls_action_pointer->setChecked(true);
  connect(view_controls_action_pointer, &QAction::toggled, this,
          &Editor::set_controls_visible);

  view_menu_pointer->addAction(view_orchestra_action_pointer);
  view_orchestra_action_pointer->setCheckable(true);
  view_orchestra_action_pointer->setChecked(true);
  connect(view_orchestra_action_pointer, &QAction::toggled, this,
          &Editor::set_orchestra_visible);

  view_menu_pointer->addAction(view_chords_action_pointer);
  view_chords_action_pointer->setCheckable(true);
  view_chords_action_pointer->setChecked(true);
  connect(view_chords_action_pointer, &QAction::toggled, this,
          &Editor::set_chords_visible);

  play_selection_action_pointer->setEnabled(false);
  play_menu_pointer->addAction(play_selection_action_pointer);
  connect(play_selection_action_pointer, &QAction::triggered, this,
          &Editor::play_selected);
  play_selection_action_pointer->setShortcuts(QKeySequence::Print);

  stop_playing_action_pointer->setEnabled(true);
  play_menu_pointer->addAction(stop_playing_action_pointer);
  connect(stop_playing_action_pointer, &QAction::triggered, song_pointer,
          &Song::stop_playing);
  stop_playing_action_pointer->setShortcuts(QKeySequence::Cancel);

  edit_menu_pointer->addAction(undo_action_pointer);
  connect(undo_action_pointer, &QAction::triggered, &song_pointer->undo_stack,
          &QUndoStack::undo);
  undo_action_pointer->setShortcuts(QKeySequence::Undo);

  edit_menu_pointer->addAction(redo_action_pointer);
  connect(redo_action_pointer, &QAction::triggered, &song_pointer->undo_stack,
          &QUndoStack::redo);
  redo_action_pointer->setShortcuts(QKeySequence::Redo);

  edit_menu_pointer->addSeparator();

  copy_action_pointer->setEnabled(false);
  edit_menu_pointer->addAction(copy_action_pointer);
  copy_action_pointer->setShortcuts(QKeySequence::Copy);
  connect(copy_action_pointer, &QAction::triggered, this,
          &Editor::copy_selected);

  // TODO: factor first/before/after?

  edit_menu_pointer->addMenu(paste_menu_pointer);

  
  paste_menu_pointer->addAction(paste_before_action_pointer);
  connect(paste_before_action_pointer, &QAction::triggered, this,
          &Editor::paste_before);
  paste_before_action_pointer->setEnabled(false);

  paste_after_action_pointer->setShortcuts(QKeySequence::Paste);
  connect(paste_after_action_pointer, &QAction::triggered, this,
          &Editor::paste_after);
  paste_menu_pointer->addAction(paste_after_action_pointer);
  paste_after_action_pointer->setEnabled(false);

  paste_into_action_pointer->setEnabled(false);
  connect(paste_into_action_pointer, &QAction::triggered, this,
          &Editor::paste_into);
  paste_menu_pointer->addAction(paste_into_action_pointer);

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

  insert_into_action_pointer->setShortcuts(QKeySequence::AddTab);
  insert_into_action_pointer->setEnabled(true);
  connect(insert_into_action_pointer, &QAction::triggered, this,
          &Editor::insert_into);
  insert_menu_pointer->addAction(insert_into_action_pointer);

  remove_action_pointer->setShortcuts(QKeySequence::Delete);
  remove_action_pointer->setEnabled(false);
  connect(remove_action_pointer, &QAction::triggered, this,
          &Editor::remove_selected);
  edit_menu_pointer->addAction(remove_action_pointer);

  orchestra_text_edit_pointer->setPlainText(song_pointer->orchestra_code);
  orchestra_column_pointer->addWidget(orchestra_text_edit_pointer);
  orchestra_column_pointer->addWidget(save_orchestra_button_pointer);
  connect(save_orchestra_button_pointer, &QAbstractButton::pressed, this,
          &Editor::save_orchestra_text);

  central_column_pointer->addWidget(orchestra_box_pointer);

  central_column_pointer->addWidget(tree_view_pointer);

  controls_box_pointer->setFixedWidth(CONTROLS_WIDTH);

  resize(WINDOW_WIDTH, WINDOW_HEIGHT);

  setWindowTitle("Justly");
  setCentralWidget(central_box_pointer);
}

void Editor::copy_selected() {
  auto selected = tree_view_pointer->selectionModel()->selectedRows();
  if (selected.empty()) {
    error_empty("copy");
    return;
  }
  auto first_index = selected[0];
  copy_level = song_pointer -> const_node_from_index(first_index).get_level();
  copy(first_index.row(), selected.size(), song_pointer->parent(first_index));
  reenable_actions();
}

// TODO: align copy and play interfaces with position, rows, parent
void Editor::copy(int position, size_t rows, const QModelIndex &parent_index) {
  auto &parent_node = song_pointer->node_from_index(parent_index);
  auto &child_pointers = parent_node.child_pointers;
  copied.clear();
  for (int index = position; index < position + rows; index = index + 1) {
    copied.push_back(
        std::make_unique<TreeNode>(*(child_pointers[index]), &parent_node));
  }
}

void Editor::play_selected() {
  auto selected = tree_view_pointer->selectionModel()->selectedRows();
  if (selected.empty()) {
    error_empty("play");
    return;
  }
  auto first_index = selected[0];
  song_pointer->play(first_index.row(), selected.size(),
                      song_pointer->parent(first_index));
}

void Editor::save_default_instrument() {
  auto new_default_instrument =
      default_instrument_selector_pointer->currentText();
  if (new_default_instrument != song_pointer->default_instrument) {
    song_pointer->undo_stack.push(new DefaultInstrumentChange(
        *this, song_pointer->default_instrument, new_default_instrument));
  }
}

void Editor::set_default_instrument(const QString &default_instrument,
                                    bool should_set_box) {
  song_pointer->default_instrument = default_instrument;
  song_pointer->redisplay();
  if (should_set_box) {
    set_combo_box(*default_instrument_selector_pointer, default_instrument);
  }
}

void Editor::insert_before() {
  auto selected = tree_view_pointer->selectionModel()->selectedRows();
  if (selected.empty()) {
    error_empty("insert before");
    return;
  }
  const auto &first_index = selected[0];
  insert(first_index.row(), 1, first_index.parent());
};

void Editor::insert_after() {
  auto selected = tree_view_pointer->selectionModel()->selectedRows();
  if (selected.empty()) {
    error_empty("insert after");
    return;
  }
  const auto &last_index = selected[selected.size() - 1];
  insert(last_index.row() + 1, 1, last_index.parent());
};

void Editor::insert_into() {
  auto selected = tree_view_pointer->selectionModel()->selectedRows();
  insert(0, 1, selected.empty() ? QModelIndex() : selected[0]);
}

void Editor::paste_before() {
  auto selected = tree_view_pointer->selectionModel()->selectedRows();
  if (selected.empty()) {
    error_empty("paste before");
    return;
  }
  const auto &first_index = selected[0];
  paste(first_index.row(), first_index.parent());
}

void Editor::paste_after() {
  auto selected = tree_view_pointer->selectionModel()->selectedRows();
  if (selected.empty()) {
    error_empty("paste after");
    return;
  }
  const auto &last_index = selected[selected.size() - 1];
  paste(last_index.row() + 1, last_index.parent());
}

void Editor::paste_into() {
  auto selected = tree_view_pointer->selectionModel()->selectedRows();
  paste(0, selected.empty() ? QModelIndex() : selected[0]);
}

void Editor::set_controls_visible() {
  controls_box_pointer->setVisible(view_controls_action_pointer->isChecked());
}

void Editor::set_orchestra_visible() {
  orchestra_box_pointer->setVisible(view_orchestra_action_pointer->isChecked());
}

void Editor::set_chords_visible() {
  tree_view_pointer->setVisible(view_chords_action_pointer->isChecked());
}

void Editor::remove_selected() {
  auto selected = tree_view_pointer->selectionModel()->selectedRows();
  if (selected.empty()) {
    error_empty("remove");
    return;
  }
  const auto &first_index = selected[0];
  remove(first_index.row(), selected.size(), first_index.parent());
  reenable_actions();
}

void Editor::remove(int position, size_t rows,
                    const QModelIndex &parent_index) {
  song_pointer->undo_stack.push(
      new Remove(*song_pointer, position, rows, parent_index));
  reenable_actions();
}

void Editor::reenable_actions() {

  auto *selection_model_pointer =
      tree_view_pointer->selectionModel();

  const auto selection = selection_model_pointer->selectedRows();
  const auto parent = tree_view_pointer->currentIndex().parent();

  QItemSelection invalid;

  Q_FOREACH (const QModelIndex index, selection) {
    if (index.parent() != parent) {
      invalid.select(index, index);
    }
  }
  selection_model_pointer->blockSignals(true);
  selection_model_pointer->select(invalid, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
  selection_model_pointer->blockSignals(false);

  // revise this later
  auto totally_empty = song_pointer->root.get_child_count() == 0;
  auto selected = selection_model_pointer->selectedRows();
  auto any_selected = !(selected.isEmpty());
  auto selected_level = 0;
  auto one_empty_chord = false;
  auto copy_match = false;
  if (any_selected) {
    const auto &first_node = song_pointer->const_node_from_index(selected[0]);
    selected_level = first_node.get_level();
    copy_match = selected_level == copy_level;
    one_empty_chord = selected.size() == 1 && selected_level == chord_level &&
                      first_node.get_child_count() == 0;
  }

  play_selection_action_pointer->setEnabled(any_selected);
  insert_before_action_pointer->setEnabled(any_selected);
  insert_after_action_pointer->setEnabled(any_selected);
  remove_action_pointer->setEnabled(any_selected);
  copy_action_pointer->setEnabled(any_selected);

  paste_before_action_pointer->setEnabled(copy_match);
  paste_after_action_pointer->setEnabled(copy_match);

  insert_into_action_pointer->setEnabled(totally_empty || one_empty_chord);
  paste_into_action_pointer->setEnabled((totally_empty && copy_level == chord_level) ||
                                        (one_empty_chord && copy_level == note_level));
};

auto Editor::set_starting_key_with_slider() -> void {
  if (song_pointer->starting_key !=
      starting_key_slider_pointer->slider_pointer->value()) {
    song_pointer->undo_stack.push(new StartingKeyChange(
        *this, starting_key_slider_pointer->slider_pointer->value()));
  }
}

auto Editor::set_starting_volume_with_slider() -> void {
  if (song_pointer->starting_volume !=
      starting_volume_slider_pointer->slider_pointer->value()) {
    song_pointer->undo_stack.push(new StartingVolumeChange(
        *this, starting_volume_slider_pointer->slider_pointer->value()));
  }
}

void Editor::set_starting_tempo_with_slider() {
  if (song_pointer->starting_tempo !=
      starting_tempo_slider_pointer->slider_pointer->value()) {
    song_pointer->undo_stack.push(new StartingTempoChange(
        *this, starting_tempo_slider_pointer->slider_pointer->value()));
  }
}

void Editor::insert(int position, int rows, const QModelIndex &parent_index) {
  // insertRows will error if invalid
  song_pointer->undo_stack.push(
      new InsertEmptyRows(*song_pointer, position, rows, parent_index));
};

void Editor::paste(int position, const QModelIndex &parent_index) {
  if (!copied.empty()) {
    song_pointer->undo_stack.push(
        new Insert(*song_pointer, position, copied, parent_index));
  }
}

void Editor::save() {
  QFileDialog const dialog(this);
  auto filename = QFileDialog::getSaveFileName(
      this, tr("Save Song"),
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
      tr("Song files (*.json)"));
  if (!filename.isNull()) {
    QFile output(filename);
    if (output.open(QIODevice::WriteOnly)) {
      output.write(song_pointer->to_json().toJson());
      output.close();
    } else {
      cannot_open_error(filename);
    }
  }
}

void Editor::open() {
  QFileDialog const dialog(this);
  auto filename = QFileDialog::getOpenFileName(
      this, tr("Open Song"),
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
      tr("Song files (*.json)"));
  if (!filename.isNull()) {
    song_pointer->undo_stack.resetClean();
    QFile input(filename);
    if (input.open(QIODevice::ReadOnly)) {
      load_from(input.readAll());
      input.close();
    } else {
      cannot_open_error(filename);
    }
  }
}

void Editor::load_from(const QByteArray &song_text) {
  if (song_pointer->load_from(song_text)) {
    default_instrument_selector_pointer->clear();
    fill_combo_box(*default_instrument_selector_pointer,
                   song_pointer->instrument_pointers);
    set_combo_box(*default_instrument_selector_pointer,
                  song_pointer->default_instrument);

    starting_key_slider_pointer->slider_pointer->setValue(
        static_cast<int>(song_pointer->starting_key));
    starting_volume_slider_pointer->slider_pointer->setValue(
        static_cast<int>(song_pointer->starting_volume));
    starting_tempo_slider_pointer->slider_pointer->setValue(
        static_cast<int>(song_pointer->starting_tempo));
    orchestra_text_edit_pointer->setPlainText(song_pointer->orchestra_code);
  }
}

void Editor::save_orchestra_text() {
  auto new_orchestra_text = orchestra_text_edit_pointer->toPlainText();
  if (new_orchestra_text != song_pointer->orchestra_code) {
    std::vector<std::unique_ptr<const QString>> new_instrument_pointers;
    extract_instruments(new_instrument_pointers, new_orchestra_text);

    if (new_instrument_pointers.empty()) {
      QMessageBox::warning(nullptr, "Orchestra error",
                           "No instruments. Cannot load");
      return;
    }
    if (!song_pointer->verify_instruments(new_instrument_pointers)) {
      return;
    }
    if (!(song_pointer->verify_orchestra_text_compiles(new_orchestra_text))) {
      return;
    }
    auto &old_default_instrument = song_pointer->default_instrument;
    if (!has_instrument(new_instrument_pointers,
                        song_pointer->default_instrument)) {
      const auto &new_default_instrument = *(new_instrument_pointers[0]);
      QMessageBox::warning(nullptr, "Orchestra warning",
                           QString("Default instrument %1 no longer exists. "
                                   "Setting default to first instrument %2")
                               .arg(old_default_instrument)
                               .arg(new_default_instrument));
      song_pointer->undo_stack.push(new OrchestraChange(
          *this, song_pointer->orchestra_code, new_orchestra_text,
          old_default_instrument, new_default_instrument));
    } else {
      song_pointer->undo_stack.push(new OrchestraChange(
          *this, song_pointer->orchestra_code, new_orchestra_text,
          old_default_instrument, old_default_instrument));
    }
  }
}

void Editor::set_orchestra_text(const QString &new_orchestra_text,
                                const QString &new_default_instrument,
                                bool should_set_text) {
  song_pointer->set_orchestra_text(new_orchestra_text);
  song_pointer->default_instrument = new_default_instrument;
  default_instrument_selector_pointer->clear();
  default_instrument_selector_pointer->blockSignals(true);
  fill_combo_box(*default_instrument_selector_pointer,
                 song_pointer->instrument_pointers);
  set_combo_box(*default_instrument_selector_pointer,
                song_pointer->default_instrument);
  default_instrument_selector_pointer->blockSignals(false);
  if (should_set_text) {
    orchestra_text_edit_pointer->setPlainText(new_orchestra_text);
  }
  song_pointer->redisplay();
}
