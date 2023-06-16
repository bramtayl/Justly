#include "Editor.h"

#include <QtCore/qglobal.h>       // for QForeachContainer, qMakeForeachCont...
#include <qabstractbutton.h>      // for QAbstractButton
#include <qabstractitemview.h>    // for QAbstractItemView, QAbstractItemVie...
#include <qabstractslider.h>      // for QAbstractSlider
#include <qbytearray.h>           // for QByteArray
#include <qfile.h>                // for QFile
#include <qfiledialog.h>          // for QFileDialog
#include <qheaderview.h>          // for QHeaderView, QHeaderView::ResizeToC...
#include <qiodevice.h>            // for QIODevice
#include <qiodevicebase.h>        // for QIODeviceBase::ReadOnly, QIODeviceB...
#include <qitemselectionmodel.h>  // for QItemSelectionModel, QItemSelection
#include <qjsondocument.h>        // for QJsonDocument
#include <qkeysequence.h>         // for QKeySequence, QKeySequence::AddTab
#include <qlist.h>                // for QList<>::const_iterator
#include <qmenubar.h>             // for QMenuBar
#include <qslider.h>              // for QSlider
#include <qstandardpaths.h>       // for QStandardPaths, QStandardPaths::Doc...
#include <qundostack.h>           // for QUndoStack

#include <QMessageBox>
#include <algorithm>  // for max

#include "NoteChord.h"  // for beats_column, denominator_column
#include "TreeNode.h"   // for TreeNode
#include "Utilities.h"  // for error_empty, set_combo_box, fill_co...
#include "commands.h"   // for DefaultInstrumentChange, FrequencyC...

Editor::Editor(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags),
      instrument_delegate(ComboBoxItemDelegate(song.instrument_pointers)) {
  
  // addMenu will take ownership, so we don't have to worry about freeing
  auto* file_menu_pointer = new QMenu(tr("&File"));
  auto* edit_menu_pointer = new QMenu(tr("&Edit"));
  auto* view_menu_pointer = new QMenu(tr("&View"));
  auto* play_menu_pointer = new QMenu(tr("&Play"));
  auto* insert_menu_pointer = new QMenu(tr("&Insert"));
  auto* paste_menu_pointer = new QMenu(tr("&Paste"));

  auto* central_column_pointer = new QVBoxLayout();
  auto* controls_form_pointer = new QFormLayout();
  auto* orchestra_column_pointer = new QVBoxLayout();

  auto* open_action_pointer = new QAction(tr("&Open"));
  auto* save_action_pointer = new QAction(tr("&Save"));

  auto* stop_action_pointer = new QAction(tr("&Stop playing"));

  auto* undo_action_pointer = new QAction(tr("&Undo"));
  auto* redo_action_pointer = new QAction(tr("&Redo"));

  menuBar()->addMenu(file_menu_pointer);
  menuBar()->addMenu(edit_menu_pointer);
  menuBar()->addMenu(view_menu_pointer);
  menuBar()->addMenu(play_menu_pointer);

  central_box.setLayout(central_column_pointer);
  orchestra_box.setLayout(orchestra_column_pointer);

  controls_form_pointer->setSizeConstraint(QLayout::SetFixedSize);

  controls_box_pointer->setLayout(controls_form_pointer);

  starting_key_slider.slider.setValue(song.starting_key);
  connect(&(starting_key_slider.slider), &QAbstractSlider::valueChanged, this,
          &Editor::set_starting_key_with_slider);
  controls_form_pointer->addRow(new QLabel(tr("Starting key")), &starting_key_slider);

  starting_volume_slider.slider.setValue(song.starting_volume);
  connect(&(starting_volume_slider.slider), &QAbstractSlider::valueChanged,
          this, &Editor::set_starting_volume_with_slider);
  controls_form_pointer->addRow(new QLabel(tr("Starting volume")), &starting_volume_slider);

  starting_tempo_slider.slider.setValue(song.starting_tempo);
  connect(&(starting_tempo_slider.slider), &QAbstractSlider::valueChanged, this,
          &Editor::set_starting_tempo_with_slider);
  controls_form_pointer->addRow(new QLabel(tr("Starting tempo")), &starting_tempo_slider);

  fill_combo_box(default_instrument_selector, song.instrument_pointers);
  set_combo_box(default_instrument_selector, song.default_instrument);
  connect(&default_instrument_selector, &QComboBox::activated, this,
          &Editor::save_default_instrument);
  controls_form_pointer->addRow(new QLabel(tr("Default instrument")), &default_instrument_selector);

  central_column_pointer->addWidget(controls_box_pointer);

  tree_view.setModel(&song);
  tree_view.setSelectionMode(QAbstractItemView::ContiguousSelection);
  tree_view.setSelectionBehavior(QAbstractItemView::SelectRows);
  connect(tree_view.selectionModel(), &QItemSelectionModel::selectionChanged, this,
          &Editor::reenable_actions);
  tree_view.header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  tree_view.setItemDelegateForColumn(numerator_column, &numerator_delegate);
  tree_view.setItemDelegateForColumn(denominator_column, &denominator_delegate);
  tree_view.setItemDelegateForColumn(octave_column, &octave_delegate);
  tree_view.setItemDelegateForColumn(beats_column, &beats_delegate);
  tree_view.setItemDelegateForColumn(volume_percent_column, &volume_delegate);
  tree_view.setItemDelegateForColumn(tempo_percent_column, &tempo_delegate);
  tree_view.setItemDelegateForColumn(instrument_column, &instrument_delegate);

  file_menu_pointer->addAction(open_action_pointer);
  connect(open_action_pointer, &QAction::triggered, this, &Editor::open);
  open_action_pointer->setShortcuts(QKeySequence::Open);

  file_menu_pointer->addAction(save_action_pointer);
  connect(save_action_pointer, &QAction::triggered, this, &Editor::save);
  save_action_pointer->setShortcuts(QKeySequence::Save);

  view_menu_pointer->addAction(view_controls_action_pointer);
  view_controls_action_pointer->setCheckable(true);
  view_controls_action_pointer->setChecked(true);
  connect(view_controls_action_pointer, &QAction::triggered, this, &Editor::set_controls_visible);
  
  view_menu_pointer->addAction(view_orchestra_action_pointer);
  view_orchestra_action_pointer->setCheckable(true);
  view_orchestra_action_pointer->setChecked(true);
  connect(view_orchestra_action_pointer, &QAction::triggered, this, &Editor::set_orchestra_visible);

  view_menu_pointer->addAction(view_chords_action_pointer);
  view_chords_action_pointer->setCheckable(true);
  view_chords_action_pointer->setChecked(true);
  connect(view_chords_action_pointer, &QAction::triggered, this, &Editor::set_chords_visible);

  play_action_pointer->setEnabled(false);
  play_menu_pointer->addAction(play_action_pointer);
  connect(play_action_pointer, &QAction::triggered, this, &Editor::play_selected);
  play_action_pointer->setShortcuts(QKeySequence::Print);

  stop_action_pointer->setEnabled(true);
  play_menu_pointer->addAction(stop_action_pointer);
  connect(stop_action_pointer, &QAction::triggered, &song, &Song::stop_playing);
  stop_action_pointer->setShortcuts(QKeySequence::Cancel);

  edit_menu_pointer->addAction(undo_action_pointer);
  connect(undo_action_pointer, &QAction::triggered, &song.undo_stack,
          &QUndoStack::undo);
  undo_action_pointer->setShortcuts(QKeySequence::Undo);

  edit_menu_pointer->addAction(redo_action_pointer);
  connect(redo_action_pointer, &QAction::triggered, &song.undo_stack,
          &QUndoStack::redo);
  redo_action_pointer->setShortcuts(QKeySequence::Redo);

  copy_action_pointer->setEnabled(false);
  edit_menu_pointer->addAction(copy_action_pointer);
  copy_action_pointer->setShortcuts(QKeySequence::Copy);
  connect(copy_action_pointer, &QAction::triggered, this, &Editor::copy_selected);

  // TODO: factor first/before/after?

  edit_menu_pointer->addMenu(paste_menu_pointer);

  paste_before_action_pointer->setEnabled(false);
  paste_menu_pointer->addAction(paste_before_action_pointer);
  connect(paste_before_action_pointer, &QAction::triggered, this,
          &Editor::paste_before);

  paste_after_action_pointer->setEnabled(false);

  paste_after_action_pointer->setShortcuts(QKeySequence::Paste);
  connect(paste_after_action_pointer, &QAction::triggered, this, &Editor::paste_after);
  paste_menu_pointer->addAction(paste_after_action_pointer);

  paste_into_action_pointer->setEnabled(true);
  connect(paste_into_action_pointer, &QAction::triggered, this, &Editor::paste_into);
  paste_menu_pointer->addAction(paste_into_action_pointer);

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
  connect(insert_into_action_pointer, &QAction::triggered, this, &Editor::insert_into);
  insert_menu_pointer->addAction(insert_into_action_pointer);

  remove_action_pointer->setShortcuts(QKeySequence::Delete);
  remove_action_pointer->setEnabled(false);
  connect(remove_action_pointer, &QAction::triggered, this, &Editor::remove_selected);
  edit_menu_pointer->addAction(remove_action_pointer);

  orchestra_text_edit.setPlainText(song.orchestra_code);
  orchestra_column_pointer->addWidget(&orchestra_text_edit);
  orchestra_column_pointer->addWidget(&save_orchestra_button);
  connect(&save_orchestra_button, &QAbstractButton::pressed, this,
          &Editor::save_orchestra_text);

  central_column_pointer->addWidget(&orchestra_box);

  central_column_pointer->addWidget(&tree_view);

  setWindowTitle("Justly");
  setCentralWidget(&central_box);
  resize(WINDOW_WIDTH, WINDOW_HEIGHT);
}

Editor::~Editor() {
  central_box.setParent(nullptr);
  tree_view.setParent(nullptr);
  starting_key_slider.setParent(nullptr);
  starting_volume_slider.setParent(nullptr);
  starting_tempo_slider.setParent(nullptr);
  save_orchestra_button.setParent(nullptr);
  orchestra_text_edit.setParent(nullptr);
  save_orchestra_button.setParent(nullptr);
}

void Editor::copy_selected() {
  selected = tree_view.selectionModel()->selectedRows();
  if (selected.empty()) {
    error_empty();
    return;
  }
  auto first_index = selected[0];
  copy(first_index.row(), selected.size(), song.parent(first_index));
}

// TODO: align copy and play interfaces with position, rows, parent
void Editor::copy(int position, size_t rows, const QModelIndex &parent_index) {
  auto &parent_node = song.node_from_index(parent_index);
  auto &child_pointers = parent_node.child_pointers;
  copied.clear();
  for (int index = position; index < position + rows; index = index + 1) {
    copied.push_back(
        std::make_unique<TreeNode>(*(child_pointers[index]), &parent_node));
  }
}

void Editor::play_selected() {
  selected = tree_view.selectionModel()->selectedRows();
  if (!(selected.empty())) {
    auto first_index = selected[0];
    song.play(first_index.row(), selected.size(), song.parent(first_index));
  }
}

void Editor::save_default_instrument() {
  auto new_default_instrument = default_instrument_selector.currentText();
  if (new_default_instrument != song.default_instrument) {
    song.undo_stack.push(new DefaultInstrumentChange(
        *this, song.default_instrument, new_default_instrument));
  }
}

void Editor::set_default_instrument(const QString &default_instrument,
                                    bool should_set_box) {
  song.default_instrument = default_instrument;
  song.redisplay();
  if (should_set_box) {
    set_combo_box(default_instrument_selector, default_instrument);
  }
}

void Editor::insert_before() {
  selected = tree_view.selectionModel()->selectedRows();
  if (selected.empty()) {
    error_empty();
    return;
  }
  const auto &first_index = selected[0];
  insert(first_index.row(), 1, first_index.parent());
};

void Editor::insert_after() {
  selected = tree_view.selectionModel()->selectedRows();
  if (selected.empty()) {
    error_empty();
    return;
  }
  const auto &last_index = selected[selected.size() - 1];
  insert(last_index.row() + 1, 1, last_index.parent());
};

void Editor::insert_into() {
  selected = tree_view.selectionModel()->selectedRows();
  insert(0, 1, selected.empty() ? QModelIndex() : selected[0]);
}

void Editor::paste_before() {
  selected = tree_view.selectionModel()->selectedRows();
  if (selected.empty()) {
    error_empty();
    return;
  }
  const auto &first_index = selected[0];
  paste(first_index.row(), first_index.parent());
}

void Editor::paste_after() {
  selected = tree_view.selectionModel()->selectedRows();
  if (selected.empty()) {
    error_empty();
    return;
  }
  const auto &last_index = selected[selected.size() - 1];
  paste(last_index.row() + 1, last_index.parent());
}

void Editor::paste_into() {
  selected = tree_view.selectionModel()->selectedRows();
  paste(0, selected.empty() ? QModelIndex() : selected[0]);
}

void Editor::set_controls_visible() {
  controls_box_pointer->setVisible(view_controls_action_pointer->isChecked());
}

void Editor::set_orchestra_visible() {
  orchestra_box.setVisible(view_orchestra_action_pointer->isChecked());
}

void Editor::set_chords_visible() {
  tree_view.setVisible(view_chords_action_pointer->isChecked());
}

void Editor::remove_selected() {
  selected = tree_view.selectionModel()->selectedRows();
  if (selected.empty()) {
    error_empty();
    return;
  }
  const auto &first_index = selected[0];
  remove(first_index.row(), selected.size(), first_index.parent());
  reenable_actions();
}

void Editor::remove(int position, size_t rows,
                    const QModelIndex &parent_index) {
  song.undo_stack.push(new Remove(song, position, rows, parent_index));
  reenable_actions();
}

void Editor::reenable_actions() {
  if (selected.empty()) {
    return;
  }

  QItemSelectionModel *selection_model_pointer = tree_view.selectionModel();

  QItemSelection const selection = selection_model_pointer->selection();
  const QModelIndex parent = tree_view.currentIndex().parent();

  QItemSelection invalid;

  Q_FOREACH (const QModelIndex index, selection.indexes()) {
    if (index.parent() != parent) {
      invalid.select(index, index);
    }
  }

  selection_model_pointer->select(invalid, QItemSelectionModel::Deselect);

  // revise this later
  auto totally_empty = song.root.get_child_count() == 0;
  selected = selection_model_pointer->selectedRows();
  auto any_selected = !(selected.isEmpty());
  auto selected_level = 0;
  auto one_empty_chord = false;
  auto copy_match = false;
  if (any_selected) {
    const auto &first_node = song.const_node_from_index(selected[0]);
    selected_level = first_node.get_level();
    copy_match = selected_level == copy_level;
    one_empty_chord = selected.size() == 1 && selected_level == chord_level &&
                      first_node.get_child_count() == 0;
  }

  play_action_pointer->setEnabled(any_selected);
  insert_before_action_pointer->setEnabled(any_selected);
  insert_after_action_pointer->setEnabled(any_selected);
  remove_action_pointer->setEnabled(any_selected);
  copy_action_pointer->setEnabled(any_selected);

  paste_before_action_pointer->setEnabled(copy_match);
  paste_after_action_pointer->setEnabled(copy_match);

  insert_into_action_pointer->setEnabled(totally_empty || one_empty_chord);
  paste_into_action_pointer->setEnabled((totally_empty && copy_level == 1) ||
                               (one_empty_chord && copy_level == 2));
};

auto Editor::set_starting_key_with_slider() -> void {
  if (song.starting_key != starting_key_slider.slider.value()) {
    song.undo_stack.push(
        new StartingKeyChange(*this, starting_key_slider.slider.value()));
  }
}

auto Editor::set_starting_volume_with_slider() -> void {
  if (song.starting_volume != starting_volume_slider.slider.value()) {
    song.undo_stack.push(
        new StartingVolumeChange(*this, starting_volume_slider.slider.value()));
  }
}

void Editor::set_starting_tempo_with_slider() {
  if (song.starting_tempo != starting_tempo_slider.slider.value()) {
    song.undo_stack.push(
        new StartingTempoChange(*this, starting_tempo_slider.slider.value()));
  }
}

void Editor::insert(int position, int rows, const QModelIndex &parent_index) {
  // insertRows will error if invalid
  song.undo_stack.push(new InsertEmptyRows(song, position, rows, parent_index));
};

void Editor::paste(int position, const QModelIndex &parent_index) {
  if (!copied.empty()) {
    song.undo_stack.push(new Insert(song, position, copied, parent_index));
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
      output.write(song.to_json().toJson());
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
    song.undo_stack.resetClean();
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
  if (song.load_from(song_text)) {
    default_instrument_selector.clear();
    fill_combo_box(default_instrument_selector, song.instrument_pointers);
    set_combo_box(default_instrument_selector, song.default_instrument);

    starting_key_slider.slider.setValue(song.starting_key);
    starting_volume_slider.slider.setValue(song.starting_volume);
    starting_tempo_slider.slider.setValue(song.starting_tempo);
    orchestra_text_edit.setPlainText(song.orchestra_code);
  }
}

void Editor::save_orchestra_text() {
  auto new_orchestra_text = orchestra_text_edit.toPlainText();
  std::vector<std::unique_ptr<const QString>> new_instrument_pointers;
  extract_instruments(new_instrument_pointers, new_orchestra_text);

  if (new_instrument_pointers.empty()) {
    QMessageBox::warning(nullptr, "Orchestra error",
                         "No instruments. Cannot load");
    return;
  }
  if (!song.verify_instruments(new_instrument_pointers, true)) {
    return;
  }
  if (!(song.verify_orchestra_text_compiles(new_orchestra_text))) {
    return;
  }
  auto& old_default_instrument = song.default_instrument;
  if (!has_instrument(new_instrument_pointers, song.default_instrument)) {
    auto& new_default_instrument = *(song.instrument_pointers[0]);
    QMessageBox::warning(nullptr, "Orchestra warning",
                         QString("Default instrument %1 no longer exists. "
                                 "Setting default to first instrument %2")
                             .arg(old_default_instrument)
                             .arg(new_default_instrument));
    song.undo_stack.push(
      new OrchestraChange(*this, song.orchestra_code, new_orchestra_text,
                          old_default_instrument, new_default_instrument));
  } else {
    song.undo_stack.push(
      new OrchestraChange(*this, song.orchestra_code, new_orchestra_text,
                          old_default_instrument, old_default_instrument));
  }
}

void Editor::set_orchestra_text(const QString &new_orchestra_text, const QString &new_default_instrument,
                                bool should_set_text) {
  song.set_orchestra_text(new_orchestra_text);
  song.default_instrument = new_default_instrument;
  default_instrument_selector.clear();
  default_instrument_selector.blockSignals(true);
  fill_combo_box(default_instrument_selector, song.instrument_pointers);
  set_combo_box(default_instrument_selector, song.default_instrument);
  default_instrument_selector.blockSignals(false);
  if (should_set_text) {
    orchestra_text_edit.setPlainText(new_orchestra_text);
  }
  song.redisplay();
}