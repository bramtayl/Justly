#include "Editor.h"

#include <QtCore/qglobal.h>        // for qCritical, qInfo
#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <qabstractbutton.h>       // for QAbstractButton
#include <qabstractitemview.h>     // for QAbstractItemView, QAbstractItemVi...
#include <qabstractslider.h>       // for QAbstractSlider
#include <qbytearray.h>            // for QByteArray
#include <qfiledialog.h>           // for QFileDialog
#include <qheaderview.h>           // for QHeaderView, QHeaderView::ResizeTo...
#include <qitemselectionmodel.h>   // for QItemSelectionModel
#include <qkeysequence.h>          // for QKeySequence, QKeySequence::AddTab
#include <qmenubar.h>              // for QMenuBar
#include <qmessagebox.h>           // for QMessageBox
#include <qslider.h>               // for QSlider
#include <qstandardpaths.h>        // for QStandardPaths, QStandardPaths::Do...
#include <qtextstream.h>           // for QTextStream, operator<<, endl

#include <algorithm>  // for max

#include "Chord.h"       // for CHORD_LEVEL
#include "Note.h"        // for NOTE_LEVEL
#include "NoteChord.h"   // for NoteChord, beats_column, denominat...
#include "TreeNode.h"    // for TreeNode
#include "Utilities.h"   // for set_combo_box, assert_not_empty
#include "commands.h"    // for CellChange, DefaultInstrumentChange

#include <csound/csPerfThread.hpp>
#include <csound/csound.hpp>  // for CSOUND

Editor::Editor(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags),
      instrument_delegate(ComboBoxItemDelegate(song.instrument_pointers)) {
  connect(&song, &Song::set_data_signal, this, &Editor::setData);

  menuBar()->addAction(file_menu.menuAction());
  menuBar()->addAction(edit_menu.menuAction());
  menuBar()->addAction(play_menu.menuAction());

  central_box.setLayout(&central_column);

  central_column.addWidget(&sliders_box);

  sliders_box.setLayout(&sliders_form);

  connect(&(frequency_slider.slider), &QAbstractSlider::sliderReleased, this,
          &Editor::set_frequency_with_slider);
  frequency_slider.slider.setValue(song.frequency);
  sliders_form.addRow(&frequency_label, &frequency_slider);

  connect(&(volume_percent_slider.slider), &QAbstractSlider::sliderReleased,
          this, &Editor::set_volume_percent_with_slider);
  volume_percent_slider.slider.setValue(song.volume_percent);
  sliders_form.addRow(&volume_percent_label, &volume_percent_slider);

  connect(&(tempo_slider.slider), &QAbstractSlider::sliderReleased, this,
          &Editor::set_tempo_with_slider);
  tempo_slider.slider.setValue(song.tempo);
  sliders_form.addRow(&tempo_label, &tempo_slider);

  sliders_form.addRow(&orchestra_text_label, &save_orchestra_button);

  fill_combo_box(default_instrument_selector, song.instrument_pointers);
  set_combo_box(default_instrument_selector, song.default_instrument);
  connect(&default_instrument_selector, &QComboBox::activated, this,
          &Editor::save_default_instrument);
  sliders_form.addRow(&default_instrument_label, &default_instrument_selector);

  view.setModel(&song);
  view.setSelectionModel(&selector);
  view.setSelectionMode(QAbstractItemView::ContiguousSelection);
  view.setSelectionBehavior(QAbstractItemView::SelectRows);
  view.header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  view.setItemDelegateForColumn(numerator_column, &numerator_delegate);
  view.setItemDelegateForColumn(denominator_column, &denominator_delegate);
  view.setItemDelegateForColumn(octave_column, &octave_delegate);
  view.setItemDelegateForColumn(beats_column, &beats_delegate);
  view.setItemDelegateForColumn(volume_percent_column, &volume_delegate);
  view.setItemDelegateForColumn(tempo_percent_column, &tempo_delegate);
  view.setItemDelegateForColumn(instrument_column, &instrument_delegate);
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

  csound_session.SetOption("--output=devaudio");
  csound_session.SetOption("--messagelevel=16");
  auto orchestra_error_code = csound_session.CompileOrc(qUtf8Printable(song.orchestra_text));
  if (orchestra_error_code != 0) {
    qCritical("Cannot compile orchestra, error code %d", orchestra_error_code);
  }
  csound_session.Start();
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

  if (performance_thread.GetStatus() == 0) {
    performance_thread.Stop();
  }
  performance_thread.Join();
}

void Editor::copy_selected() {
  selected = view.selectionModel()->selectedRows();
  if (!(selected.empty())) {
    auto first_index = selected[0];
    copy(first_index.row(), selected.size(), song.parent(first_index));
  }
}

// TODO: align copy and play interfaces with position, rows, parent
void Editor::copy(int position, size_t rows, const QModelIndex &parent_index) {
  auto &parent_node = song.node_from_index(parent_index);
  copy_level = parent_node.get_level() + 1;
  auto &child_pointers = parent_node.child_pointers;
  copied.clear();
  for (int index = position; index < position + rows; index = index + 1) {
    copied.push_back(
        std::make_unique<TreeNode>(*(child_pointers[index]), &parent_node));
  }
}

void Editor::play_selected() {
  selected = view.selectionModel()->selectedRows();
  if (!(selected.empty())) {
    auto first_index = selected[0];
    play(first_index.row(), selected.size(), song.parent(first_index));
  }
}

void Editor::stop_playing() {
  performance_thread.Pause();
  performance_thread.FlushMessageQueue();
  csound_session.RewindScore();
}

void Editor::save_default_instrument() {
  auto new_default_instrument = default_instrument_selector.currentText();
  if (new_default_instrument != song.default_instrument) {
    undo_stack.push(new DefaultInstrumentChange(*this, song.default_instrument,
                                                new_default_instrument));
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

void Editor::play(int position, size_t rows, const QModelIndex &parent_index) {
  stop_playing();

  key = song.frequency;
  current_volume = (FULL_NOTE_VOLUME * song.volume_percent) / PERCENT;
  current_tempo = song.tempo;
  current_time = 0.0;

  auto end_position = position + rows;
  auto &parent = song.node_from_index(parent_index);
  parent.assert_child_at(position);
  parent.assert_child_at(end_position - 1);
  auto &sibling_pointers = parent.child_pointers;
  auto level = parent.get_level() + 1;
  if (level == CHORD_LEVEL) {
    for (auto index = 0; index < position; index = index + 1) {
      auto &sibling = *sibling_pointers[index];
      update_with_chord(sibling);
    }
    for (auto index = position; index < end_position; index = index + 1) {
      auto &sibling = *sibling_pointers[index];
      update_with_chord(sibling);
      for (const auto &nibling_pointer : sibling.child_pointers) {
        schedule_note(*nibling_pointer);
      }
      current_time = current_time +
                      get_beat_duration() * sibling.note_chord_pointer->beats;
    }
  } else if (level == NOTE_LEVEL) {
    auto &grandparent = parent.get_parent();
    auto &uncle_pointers = grandparent.child_pointers;
    auto parent_position = parent.is_at_row();
    grandparent.assert_child_at(parent_position);
    for (auto index = 0; index <= parent_position; index = index + 1) {
      update_with_chord(*uncle_pointers[index]);
    }
    for (auto index = position; index < end_position; index = index + 1) {
      schedule_note(*sibling_pointers[index]);
    }
  } else {
    qCritical("Invalid level %d!", level);
  }
  
  performance_thread.Play();
}

auto Editor::first_selected_index() -> QModelIndex {
  selected = view.selectionModel()->selectedRows();
  assert_not_empty(selected);
  return selected[0];
}

auto Editor::last_selected_index() -> QModelIndex {
  selected = view.selectionModel()->selectedRows();
  assert_not_empty(selected);
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
  assert_not_empty(selected);
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

auto Editor::set_frequency_with_slider() -> void {
  if (song.frequency != frequency_slider.slider.value()) {
    undo_stack.push(
        new FrequencyChange(*this, frequency_slider.slider.value()));
  }
}

auto Editor::set_volume_percent_with_slider() -> void {
  if (song.volume_percent != volume_percent_slider.slider.value()) {
    undo_stack.push(
        new VolumeChange(*this, volume_percent_slider.slider.value()));
  }
}

auto Editor::set_tempo_with_slider() -> void {
  if (song.tempo != tempo_slider.slider.value()) {
    undo_stack.push(new TempoChange(*this, tempo_slider.slider.value()));
  }
}

auto Editor::setData(const QModelIndex &index, const QVariant &value) -> bool {
  undo_stack.push(new CellChange(song, index, value));
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

void Editor::load_from(const QString &file) {
  song.load_from(file);

  default_instrument_selector.clear();
  fill_combo_box(default_instrument_selector, song.instrument_pointers);
  set_combo_box(default_instrument_selector, song.default_instrument);

  frequency_slider.slider.setValue(song.frequency);
  volume_percent_slider.slider.setValue(song.volume_percent);
  tempo_slider.slider.setValue(song.tempo);
  orchestra_text_edit.setPlainText(song.orchestra_text);
}

void Editor::update_with_chord(const TreeNode &node) {
  const auto &note_chord_pointer = node.note_chord_pointer;
  key = key * node.get_ratio();
  current_volume = current_volume * note_chord_pointer->volume_percent / 100.0;
  current_tempo = current_tempo * note_chord_pointer->tempo_percent / 100.0;
}

auto Editor::get_beat_duration() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}

void Editor::schedule_note(const TreeNode &node) {
  auto *note_chord_pointer = node.note_chord_pointer.get();
  auto instrument = note_chord_pointer->instrument;
  performance_thread.InputMessage(qUtf8Printable(QString("i \"%1\" %2 %3 %4 %5").arg(
    instrument
  ).arg(
    current_time
  ).arg(
    get_beat_duration() * note_chord_pointer->beats *
                   note_chord_pointer->tempo_percent / 100.0
  ).arg(
    key * node.get_ratio()

  ).arg(
    current_volume * note_chord_pointer->volume_percent / 100.0
  )));
}

void Editor::fill_default_instrument_options() {
  for (int index = 0; index < song.instrument_pointers.size();
       index = index + 1) {
    default_instrument_selector.insertItem(
        index, *(song.instrument_pointers.at(index)));
  }
}

void Editor::save_orchestra_text() {
  auto new_orchestra_text = orchestra_text_edit.toPlainText();
  std::vector<std::unique_ptr<const QString>> new_instrument_pointers;
  extract_instruments(new_instrument_pointers, new_orchestra_text);

  auto missing_instrument =
      song.find_missing_instrument(new_instrument_pointers);
  if (!(missing_instrument.isNull())) {
    QMessageBox::warning(nullptr, "Instrument warning",
                         QString("Cannot find instrument ") +
                             missing_instrument +
                             "! Not changing orchestra text");
    return;
  }
  // test the orchestra
  stop_playing();
  auto orchestra_error_code = csound_session.CompileOrc(qUtf8Printable(new_orchestra_text));
  if (orchestra_error_code != 0) {
    QMessageBox::warning(nullptr, "Orchestra warning",
                         QString("Cannot compile orchestra, error code %1! Not changing orchestra text").arg(orchestra_error_code)
    );
    return;
  }
  // undo, then redo later
  // TODO: only do this once?
  csound_session.CompileOrc(qUtf8Printable(song.orchestra_text));
  undo_stack.push(
    new OrchestraChange(*this, song.orchestra_text, new_orchestra_text));
}

void Editor::set_orchestra_text(const QString &new_orchestra_text,
                                bool should_set_text) {
  song.orchestra_text = new_orchestra_text;
  song.instrument_pointers.clear();
  extract_instruments(song.instrument_pointers, new_orchestra_text);
  default_instrument_selector.clear();
  fill_combo_box(default_instrument_selector, song.instrument_pointers);
  set_combo_box(default_instrument_selector, song.default_instrument);
  if (should_set_text) {
    orchestra_text_edit.setPlainText(new_orchestra_text);
  }
  csound_session.CompileOrc(qUtf8Printable(song.orchestra_text));
  song.redisplay();
}
