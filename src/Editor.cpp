#include "Editor.h"

#include <QtCore/qglobal.h>       // for qCritical
#include <qabstractitemview.h>    // for QAbstractItemView, QAbstractItemVie...
#include <qabstractslider.h>      // for QAbstractSlider
#include <qbytearray.h>           // for QByteArray
#include <qcontainerfwd.h>        // for QStringList
#include <qdialog.h>              // for QDialog, QDialog::Accepted
#include <qfile.h>                // for QFile
#include <qfiledialog.h>          // for QFileDialog, QFileDialog::Accept
#include <qheaderview.h>          // for QHeaderView, QHeaderView::ResizeToC...
#include <qiodevice.h>            // for QIODevice
#include <qiodevicebase.h>        // for QIODeviceBase::ReadOnly
#include <qitemselectionmodel.h>  // for QItemSelectionModel
#include <qjsondocument.h>        // for QJsonDocument
#include <qkeysequence.h>         // for QKeySequence, QKeySequence::AddTab
#include <qlist.h>                // for QList
#include <qmenubar.h>             // for QMenuBar
#include <qobject.h>              // for QObject
#include <qregularexpression.h>   // for QRegularExpressionMatchIteratorRang...
#include <qstandardpaths.h>       // for QStandardPaths, QStandardPaths::Doc...
#include <qtextstream.h>          // for QTextStream

#include <algorithm>  // for max
#include <string>     // for string
#include <utility>    // for move

#include "CsoundData.h"   // for CsoundData
#include "JsonHelpers.h"  // for get_positive_int, get_non_negative_int
#include "TreeNode.h"     // for TreeNode
#include "commands.h"     // for CellChange, FrequencyChange, Insert

Editor::Editor(const QString &orchestra_file, const QString& default_instrument, QWidget *parent,
               Qt::WindowFlags flags)
    : QMainWindow(parent, flags),
      player(Player(orchestra_file)),
      song(Song(orchestra_file, default_instrument)) {
  connect(&song, &Song::set_data_signal, this, &Editor::setData);

  (*menuBar()).addAction(menu_tab.menuAction());

  central_box.setLayout(&central_column);

  central_column.addWidget(&sliders_box);

  sliders_box.setLayout(&sliders_form);

  frequency_slider.setRange(MIN_FREQUENCY, MAX_FREQUENCY);
  connect(&frequency_slider, &QAbstractSlider::valueChanged, this,
          &Editor::set_frequency_label);
  connect(&frequency_slider, &QAbstractSlider::sliderReleased, this,
          &Editor::set_frequency);
  frequency_slider.setValue(song.frequency);
  sliders_form.addRow(&frequency_label, &frequency_slider);

  volume_percent_slider.setRange(MIN_VOLUME_PERCENT, MAX_VOLUME_PERCENT);
  connect(&volume_percent_slider, &QAbstractSlider::valueChanged, this,
          &Editor::set_volume_percent_label);
  connect(&volume_percent_slider, &QAbstractSlider::sliderReleased, this,
          &Editor::set_volume_percent);
  volume_percent_slider.setValue(song.volume_percent);
  sliders_form.addRow(&volume_percent_label, &volume_percent_slider);

  tempo_slider.setRange(MIN_TEMPO, MAX_TEMPO);
  connect(&tempo_slider, &QAbstractSlider::valueChanged, this,
          &Editor::set_tempo_label);
  connect(&tempo_slider, &QAbstractSlider::sliderReleased, this,
          &Editor::set_tempo);
  tempo_slider.setValue(song.tempo);
  sliders_form.addRow(&tempo_label, &tempo_slider);

  view.setModel(&song);
  view.setSelectionModel(&selector);
  view.setSelectionMode(QAbstractItemView::ContiguousSelection);
  view.setSelectionBehavior(QAbstractItemView::SelectRows);
  view.header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  connect(&selector, &QItemSelectionModel::selectionChanged, this,
          &Editor::reenable_actions);

  menu_tab.addAction(insert_menu.menuAction());

  insert_before_action.setEnabled(false);
  connect(&insert_before_action, &QAction::triggered, this,
          &Editor::insert_before);
  insert_menu.addAction(&insert_before_action);

  insert_after_action.setEnabled(false);
  insert_after_action.setShortcuts(QKeySequence::InsertLineSeparator);
  connect(&insert_after_action, &QAction::triggered, this,
          &Editor::insert_after);
  insert_menu.addAction(&insert_after_action);

  insert_into_action.setShortcuts(QKeySequence::AddTab);
  insert_into_action.setEnabled(true);
  connect(&insert_into_action, &QAction::triggered, this, &Editor::insert_into);
  insert_menu.addAction(&insert_into_action);

  remove_action.setShortcuts(QKeySequence::Delete);
  remove_action.setEnabled(false);
  connect(&remove_action, &QAction::triggered, this, &Editor::remove_selected);
  menu_tab.addAction(&remove_action);

  play_action.setEnabled(false);
  menu_tab.addAction(&play_action);
  connect(&play_action, &QAction::triggered, this, &Editor::play_selected);
  play_action.setShortcuts(QKeySequence::Print);

  stop_action.setEnabled(true);
  menu_tab.addAction(&stop_action);
  connect(&stop_action, &QAction::triggered, this, &Editor::stop_playing);
  stop_action.setShortcuts(QKeySequence::Cancel);

  menu_tab.addAction(&undo_action);
  connect(&undo_action, &QAction::triggered, &undo_stack, &QUndoStack::undo);
  undo_action.setShortcuts(QKeySequence::Undo);

  menu_tab.addAction(&redo_action);
  connect(&redo_action, &QAction::triggered, &undo_stack, &QUndoStack::redo);
  redo_action.setShortcuts(QKeySequence::Redo);

  copy_action.setEnabled(false);
  menu_tab.addAction(&copy_action);
  copy_action.setShortcuts(QKeySequence::Copy);
  connect(&copy_action, &QAction::triggered, this, &Editor::copy_selected);

  // TODO: factor first/before/after?

  menu_tab.addAction(paste_menu.menuAction());

  paste_before_action.setEnabled(false);
  paste_menu.addAction(&paste_before_action);
  connect(&paste_before_action, &QAction::triggered, this,
          &Editor::paste_before);

  paste_after_action.setEnabled(false);

  paste_after_action.setShortcuts(QKeySequence::Paste);
  connect(&paste_after_action, &QAction::triggered, this, &Editor::paste_after);
  paste_menu.addAction(&paste_after_action);

  paste_into_action.setEnabled(true);
  connect(&paste_into_action, &QAction::triggered, this, &Editor::paste_into);
  paste_menu.addAction(&paste_into_action);

  central_column.addWidget(&view);

  setWindowTitle("Justly");
  setCentralWidget(&central_box);
  resize(WINDOW_WIDTH, WINDOW_HEIGHT);
}

Editor::~Editor() {
  central_box.setParent(nullptr);
  view.setParent(nullptr);
  sliders_box.setParent(nullptr);
  frequency_slider.setParent(nullptr);
  volume_percent_slider.setParent(nullptr);
  tempo_slider.setParent(nullptr);
}

void Editor::copy_selected() {
  selected = view.selectionModel()->selectedRows();
  if (!(selected.empty())) {
    copy(selected[0], selected.size());
  }
}

// TODO: align copy and play interfaces with position, rows, parent
void Editor::copy(const QModelIndex &first_index, size_t rows) {
  const auto &node = song.const_node_from_index(first_index);
  copy_level = node.get_level();
  auto &parent_node = node.get_parent();
  auto &child_pointers = parent_node.child_pointers;
  auto first_row = first_index.row();
  copied.clear();
  for (int offset = 0; offset < rows; offset = offset + 1) {
    copied.push_back(std::make_unique<TreeNode>(
        *(child_pointers[first_row + offset]), &parent_node));
  }
}

void Editor::play_selected() {
  selected = view.selectionModel()->selectedRows();
  if (!(selected.empty())) {
    play(selected[0], selected.size());
  }
}

void Editor::stop_playing() { player.csound_data.stop_song(); }

void Editor::play(const QModelIndex &first_index, size_t rows) {
  player.play(song, first_index, rows);
}

void Editor::assert_not_empty(const QModelIndexList &selected) {
  if (selected.empty()) {
    qCritical("Empty selected");
  }
}

auto Editor::first_selected_index() -> QModelIndex {
  selected = view.selectionModel()->selectedRows();
  Editor::assert_not_empty(selected);
  return selected[0];
}

auto Editor::last_selected_index() -> QModelIndex {
  selected = view.selectionModel()->selectedRows();
  Editor::assert_not_empty(selected);
  return selected[selected.size() - 1];
}

auto Editor::selection_parent_or_root_index() -> QModelIndex {
  selected = view.selectionModel()->selectedRows();
  if (selected.empty()) {
    return {};
  }
  return selected[0].parent();
}

void Editor::insert_before() {
  const auto &first_index = first_selected_index();
  insert(first_index.row(), 1, first_index.parent());
};

void Editor::insert_after() {
  const auto &last_index = last_selected_index();
  insert(last_index.row() + 1, 1, last_index.parent());
};

void Editor::insert_into() {
  selected = view.selectionModel()->selectedRows();
  insert(0, 1, selected.empty() ? QModelIndex() : selected[0]);
}

void Editor::paste_before() {
  const auto &first_index = first_selected_index();
  paste(first_index.row(), first_index.parent());
}

void Editor::paste_after() {
  const auto &last_index = last_selected_index();
  paste(last_index.row() + 1, last_index.parent());
}

void Editor::paste_into() {
  selected = view.selectionModel()->selectedRows();
  paste(0, selected.empty() ? QModelIndex() : selected[0]);
}

void Editor::remove_selected() {
  selected = view.selectionModel()->selectedRows();
  Editor::assert_not_empty(selected);
  auto &first_index = selected[0];
  remove(first_index.row(), selected.size(), first_index.parent());
  reenable_actions();
}

void Editor::remove(int position, size_t rows,
                    const QModelIndex &parent_index) {
  undo_stack.push(new Remove(song, position, rows, parent_index));
  reenable_actions();
}

void Editor::reenable_actions() {
  // revise this later
  auto totally_empty = song.root.get_child_count() == 0;
  selected = view.selectionModel()->selectedRows();
  auto any_selected = !(selected.isEmpty());
  auto selected_level = 0;
  auto one_empty_chord = false;
  auto copy_match = false;
  if (any_selected) {
    const auto &first_node = song.const_node_from_index(selected[0]);
    selected_level = first_node.get_level();
    copy_match = selected_level == copy_level;
    one_empty_chord = selected.size() == 1 && selected_level == 1 &&
                      first_node.get_child_count() == 0;
  }

  play_action.setEnabled(any_selected);
  insert_before_action.setEnabled(any_selected);
  insert_after_action.setEnabled(any_selected);
  remove_action.setEnabled(any_selected);
  copy_action.setEnabled(any_selected);

  paste_before_action.setEnabled(copy_match);
  paste_after_action.setEnabled(copy_match);

  insert_into_action.setEnabled(totally_empty || one_empty_chord);
  paste_into_action.setEnabled((totally_empty && copy_level == 1) ||
                               (one_empty_chord && copy_level == 2));
};

auto Editor::set_frequency() -> void {
  undo_stack.push(new FrequencyChange(*this, frequency_slider.value()));
}

auto Editor::set_volume_percent() -> void {
  undo_stack.push(new VolumeChange(*this, volume_percent_slider.value()));
}

auto Editor::set_tempo() -> void {
  undo_stack.push(new TempoChange(*this, tempo_slider.value()));
}

auto Editor::set_frequency_label(int value) -> void {
  frequency_label.setText(tr("Starting frequency: %1 Hz").arg(value));
}

auto Editor::set_volume_percent_label(int value) -> void {
  volume_percent_label.setText(tr("Starting volume: %1%").arg(value));
}

auto Editor::set_tempo_label(int value) -> void {
  tempo_label.setText(tr("Starting tempo: %1 bpm").arg(value));
}

auto Editor::setData(const QModelIndex &index, const QVariant &value, int role)
    -> bool {
  undo_stack.push(new CellChange(song, index, value, role));
  // this is not quite right
  return true;
};

auto Editor::insert(int position, int rows, const QModelIndex &parent_index)
    -> bool {
  // insertRows will error if invalid
  undo_stack.push(new InsertEmptyRows(song, position, rows, parent_index));
  return true;
};

void Editor::paste(int position, const QModelIndex &parent_index) {
  if (!copied.empty()) {
    undo_stack.push(new Insert(song, position, copied, parent_index));
  }
}

auto Editor::choose_file() -> QString {
  auto default_folder =
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  auto filter = QObject::tr("Song files (*.json)");

  QFileDialog open_dialog(nullptr, QObject::tr("Open song"), default_folder,
                          filter);
  open_dialog.setFileMode(QFileDialog::ExistingFile);
  open_dialog.setLabelText(QFileDialog::Accept, QObject::tr("Open"));
  open_dialog.setLabelText(QFileDialog::Reject, QObject::tr("Create new"));

  if (open_dialog.exec() == QDialog::Accepted) {
    return open_dialog.selectedFiles().at(0);
  }

  QFileDialog create_dialog(nullptr, QObject::tr("Create song"), default_folder,
                            filter);
  create_dialog.setAcceptMode(QFileDialog::AcceptSave);
  create_dialog.setLabelText(QFileDialog::Accept, QObject::tr("Create"));

  if (create_dialog.exec() == QDialog::Accepted) {
    QString song_file = create_dialog.selectedFiles().at(0);
    if (!(song_file.endsWith(".json"))) {
      song_file.append(".json");
    }
    return song_file;
  }

  return "";
}

void Editor::load(const QString &file) {
  song.load(file);
  frequency_slider.setValue(song.frequency);
  volume_percent_slider.setValue(song.volume_percent);
  tempo_slider.setValue(song.tempo);
}