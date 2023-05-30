#include "Editor.h"
#include <QtCore/qglobal.h>       // for qCritical, qInfo
#include <qabstractbutton.h>      // for QAbstractButton
#include <qabstractitemview.h>    // for QAbstractItemView, QAbstractItemVie...
#include <qabstractslider.h>      // for QAbstractSlider
#include <qbytearray.h>           // for QByteArray
#include <qdebug.h>               // for QDebug
#include <qfiledialog.h>          // for QFileDialog
#include <qheaderview.h>          // for QHeaderView, QHeaderView::ResizeToC...
#include <qitemselectionmodel.h>  // for QItemSelectionModel
#include <qkeysequence.h>         // for QKeySequence, QKeySequence::AddTab
#include <qmenubar.h>             // for QMenuBar
#include <qstandardpaths.h>       // for QStandardPaths, QStandardPaths::Doc...
#include <qtextstream.h>          // for QTextStream, operator<<, endl
#include <algorithm>              // for max
#include "Chord.h"                // for CHORD_LEVEL
#include "CsoundData.h"           // for CsoundData
#include "Note.h"                 // for NOTE_LEVEL
#include "NoteChord.h"            // for NoteChord
#include "TreeNode.h"             // for TreeNode
#include "commands.h"             // for CellChange, FrequencyChange, Insert
#include "Utilities.h"

Editor::Editor(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags) {
  connect(&song, &Song::set_data_signal, this, &Editor::setData);

  menuBar()->addAction(file_menu.menuAction());
  menuBar()->addAction(edit_menu.menuAction());
  menuBar()->addAction(play_menu.menuAction());

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

  sliders_form.addRow(&orchestra_text_label, &save_orchestra_button);

  update_default_instruments();
  reset_default_instrument();
  connect(&default_instrument_selector, &QComboBox::activated, this,
          &Editor::set_default_instrument);
  sliders_form.addRow(&default_instrument_label, &default_instrument_selector);

  view.setModel(&song);
  view.setSelectionModel(&selector);
  view.setSelectionMode(QAbstractItemView::ContiguousSelection);
  view.setSelectionBehavior(QAbstractItemView::SelectRows);
  view.header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  connect(&selector, &QItemSelectionModel::selectionChanged, this,
          &Editor::reenable_actions);

  file_menu.addAction(&open_action);
  connect(&open_action, &QAction::triggered, this, &Editor::open);
  open_action.setShortcuts(QKeySequence::Open);

  file_menu.addAction(&save_action);
  connect(&save_action, &QAction::triggered, this, &Editor::save);
  save_action.setShortcuts(QKeySequence::Save);

  edit_menu.addAction(insert_menu.menuAction());

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
  edit_menu.addAction(&remove_action);

  play_action.setEnabled(false);
  play_menu.addAction(&play_action);
  connect(&play_action, &QAction::triggered, this, &Editor::play_selected);
  play_action.setShortcuts(QKeySequence::Print);

  stop_action.setEnabled(true);
  play_menu.addAction(&stop_action);
  connect(&stop_action, &QAction::triggered, this, &Editor::stop_playing);
  stop_action.setShortcuts(QKeySequence::Cancel);

  edit_menu.addAction(&undo_action);
  connect(&undo_action, &QAction::triggered, &undo_stack, &QUndoStack::undo);
  undo_action.setShortcuts(QKeySequence::Undo);

  edit_menu.addAction(&redo_action);
  connect(&redo_action, &QAction::triggered, &undo_stack, &QUndoStack::redo);
  redo_action.setShortcuts(QKeySequence::Redo);

  copy_action.setEnabled(false);
  edit_menu.addAction(&copy_action);
  copy_action.setShortcuts(QKeySequence::Copy);
  connect(&copy_action, &QAction::triggered, this, &Editor::copy_selected);

  // TODO: factor first/before/after?

  edit_menu.addAction(paste_menu.menuAction());

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

  orchestra_text_edit.setPlainText(song.orchestra_text);
  central_column.addWidget(&orchestra_text_label);
  central_column.addWidget(&orchestra_text_edit);
  central_column.addWidget(&save_orchestra_button);
  connect(&save_orchestra_button, &QAbstractButton::pressed, this,
          &Editor::save_orchestra_text);
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
  orchestra_text_label.setParent(nullptr);
  save_orchestra_button.setParent(nullptr);
  orchestra_text_edit.setParent(nullptr);
  orchestra_text_label.setParent(nullptr);
  save_orchestra_button.setParent(nullptr);
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

void Editor::stop_playing() { csound_data.stop_song(); }

void Editor::set_default_instrument() {
  song.default_instrument = default_instrument_selector.currentText();
  song.reset();
}

void Editor::play(const QModelIndex &first_index, size_t rows) {
  if (orchestra_file.open()) {
    QTextStream orchestra_io(&orchestra_file);
    orchestra_io << song.orchestra_text;
    orchestra_file.close();
  } else {
    cannot_open_error(orchestra_file.fileName());
  }

  if (score_file.open()) {
    qInfo() << score_file.fileName();
    // file.fileName() returns the unique file name
    QTextStream csound_io(&score_file);

    key = song.frequency;
    current_volume = (FULL_NOTE_VOLUME * song.volume_percent) / PERCENT;
    current_tempo = song.tempo;
    current_time = 0.0;

    const auto &item = song.const_node_from_index(first_index);
    auto item_position = item.is_at_row();
    auto end_position = item_position + rows;
    auto &parent = item.get_parent();
    parent.assert_child_at(item_position);
    parent.assert_child_at(end_position - 1);
    auto &sibling_pointers = parent.child_pointers;
    auto level = item.get_level();
    if (level == CHORD_LEVEL) {
      for (auto index = 0; index < end_position; index = index + 1) {
        auto &sibling = *sibling_pointers[index];
        modulate(sibling);
        if (index >= item_position) {
          for (const auto &nibling_pointer : sibling.child_pointers) {
            schedule_note(csound_io, *nibling_pointer);
          }
          current_time = current_time + get_beat_duration() *
                                            sibling.note_chord_pointer->beats;
        }
      }
    } else if (level == NOTE_LEVEL) {
      auto &grandparent = parent.get_parent();
      auto &uncle_pointers = grandparent.child_pointers;
      auto parent_position = parent.is_at_row();
      grandparent.assert_child_at(parent_position);
      for (auto index = 0; index <= parent_position; index = index + 1) {
        modulate(*uncle_pointers[index]);
      }
      for (auto index = item_position; index < end_position;
           index = index + 1) {
        schedule_note(csound_io, *sibling_pointers[index]);
      }
    } else {
      qCritical("Invalid level %d!", level);
    }

    score_file.close();
  } else {
    cannot_open_error(score_file.fileName());
  }

  csound_data.stop_song();

  csound_data.start_song({"csound", "--output=devaudio",
                          qUtf8Printable(orchestra_file.fileName()), qUtf8Printable(score_file.fileName())});
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

void Editor::save_to(const QString &file) const { song.save_to(file); }

void Editor::save() {
  QFileDialog const dialog(this);
  auto filename = QFileDialog::getSaveFileName(
      this, tr("Save Song"),
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
      tr("Song files (*.json)"));
  if (!filename.isNull()) {
    save_to(filename);
  }
}

void Editor::open() {
  QFileDialog const dialog(this);
  auto filename = QFileDialog::getOpenFileName(
      this, tr("Open Song"),
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
      tr("Song files (*.json)"));
  if (!filename.isNull()) {
    load_from(filename);
  }
}

void Editor::reset_default_instrument() {
  for (int index = 0; index < song.instruments.size(); index = index + 1) {
    if (song.instruments.at(index)->compare(song.default_instrument) == 0) {
      default_instrument_selector.setCurrentIndex(index);
      return;
    }
  }
  NoteChord::error_instrument(song.default_instrument);
}

void Editor::load_from(const QString &file) {
  song.load_from(file);

  reset_default_instrument();

  frequency_slider.setValue(song.frequency);
  volume_percent_slider.setValue(song.volume_percent);
  tempo_slider.setValue(song.tempo);
}

void Editor::modulate(const TreeNode &node) {
  const auto &note_chord_pointer = node.note_chord_pointer;
  key = key * node.get_ratio();
  current_volume = current_volume * note_chord_pointer->volume_ratio;
  current_tempo = current_tempo * note_chord_pointer->tempo_ratio;
}

auto Editor::get_beat_duration() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}

void Editor::schedule_note(QTextStream &csound_io, const TreeNode &node) const {
  auto *note_chord_pointer = node.note_chord_pointer.get();
  auto instrument = note_chord_pointer->instrument;
  csound_io << "i \"";
  csound_io << qUtf8Printable(instrument);
  csound_io << "\" ";
  csound_io << current_time;
  csound_io << " ";
  csound_io << get_beat_duration() * note_chord_pointer->beats *
                   note_chord_pointer->tempo_ratio;
  csound_io << " ";
  csound_io << key * node.get_ratio();
  csound_io << " ";
  csound_io << current_volume * note_chord_pointer->volume_ratio;
  csound_io << Qt::endl;
}

void Editor::update_default_instruments() {
  for (int index = 0; index < song.instruments.size(); index = index + 1) {
    default_instrument_selector.insertItem(index,
                                           *(song.instruments.at(index)));
  }
}

void Editor::save_orchestra_text() {
  song.orchestra_text = orchestra_text_edit.toPlainText();
  song.instruments = get_instruments(song.orchestra_text);
  default_instrument_selector.clear();
  update_default_instruments();
  reset_default_instrument();
}
