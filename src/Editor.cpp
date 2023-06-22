#include "Editor.h"

#include <QtCore/qglobal.h>       // for qCritical
#include <QtCore/qtcoreexports.h> // for qUtf8Printable
#include <qabstractbutton.h>      // for QAbstractButton
#include <qabstractitemmodel.h>   // for QModelIndex
#include <qabstractitemview.h>    // for QAbstractItemView, QAbstractItemVi...
#include <qabstractslider.h>      // for QAbstractSlider
#include <qbytearray.h>           // for QByteArray
#include <qfile.h>                // for QFile
#include <qfiledialog.h>          // for QFileDialog
#include <qheaderview.h>          // for QHeaderView, QHeaderView::ResizeTo...
#include <qiodevice.h>            // for QIODevice
#include <qiodevicebase.h>        // for QIODeviceBase::ReadOnly, QIODevice...
#include <qitemselectionmodel.h>  // for QItemSelectionModel, operator|
#include <qjsondocument.h>        // for QJsonDocument
#include <qkeysequence.h>         // for QKeySequence, QKeySequence::AddTab
#include <qlabel.h>               // for QLabel
#include <qlist.h>                // for QList, QList<>::const_iterator
#include <qmenubar.h>             // for QMenuBar
#include <qmessagebox.h>          // for QMessageBox
#include <qsize.h>                // for QSize
#include <qslider.h>              // for QSlider
#include <qstandardpaths.h>       // for QStandardPaths, QStandardPaths::Do...
#include <qundostack.h>           // for QUndoStack

#include "ChordsModel.h"          // for ChordsModel
#include "ComboBoxItemDelegate.h" // for ComboBoxItemDelegate
#include "NoteChord.h"            // for NoteChord, beats_column, denominat...
#include "TreeNode.h"             // for TreeNode
#include "Utilities.h"            // for error_empty, set_combo_box, fill_c...
#include "commands.h"             // for OrchestraChange, Insert, InsertEmp...

Editor::Editor(const QString &starting_instrument_input,
               const QString &orchestra_code_input, QWidget *parent,
               Qt::WindowFlags flags)
    : song(csound_session, undo_stack, starting_instrument_input,
           orchestra_code_input),
      QMainWindow(parent, flags),
      instrument_delegate_pointer(
          new ComboBoxItemDelegate(song.instrument_pointers)) {
  csound_session.SetOption("--output=devaudio");
  csound_session.SetOption("--messagelevel=16");

  auto orchestra_error_code =
      csound_session.CompileOrc(qUtf8Printable(song.orchestra_code));
  if (orchestra_error_code != 0) {
    qCritical("Cannot compile orchestra, error code %d", orchestra_error_code);
    return;
  }

  csound_session.Start();

  file_menu_pointer->addAction(open_action_pointer);
  connect(open_action_pointer, &QAction::triggered, this, &Editor::open);
  open_action_pointer->setShortcuts(QKeySequence::Open);

  save_action_pointer->setShortcuts(QKeySequence::Save);
  connect(save_action_pointer, &QAction::triggered, this, &Editor::save);
  file_menu_pointer->addAction(save_action_pointer);

  menuBar()->addMenu(file_menu_pointer);

  view_controls_checkbox_pointer->setCheckable(true);
  view_controls_checkbox_pointer->setChecked(true);
  connect(view_controls_checkbox_pointer, &QAction::toggled, this,
          &Editor::view_controls);
  view_menu_pointer->addAction(view_controls_checkbox_pointer);

  view_orchestra_checkbox_pointer->setCheckable(true);
  view_orchestra_checkbox_pointer->setChecked(false);
  connect(view_orchestra_checkbox_pointer, &QAction::toggled, this,
          &Editor::view_orchestra);
  view_menu_pointer->addAction(view_orchestra_checkbox_pointer);

  view_chords_checkbox_pointer->setCheckable(true);
  view_chords_checkbox_pointer->setChecked(true);
  connect(view_chords_checkbox_pointer, &QAction::toggled, this,
          &Editor::view_chords);
  view_menu_pointer->addAction(view_chords_checkbox_pointer);

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

  undo_action_pointer->setShortcuts(QKeySequence::Undo);
  connect(undo_action_pointer, &QAction::triggered, &(song.undo_stack),
          &QUndoStack::undo);
  edit_menu_pointer->addAction(undo_action_pointer);

  redo_action_pointer->setShortcuts(QKeySequence::Redo);
  edit_menu_pointer->addAction(redo_action_pointer);
  connect(redo_action_pointer, &QAction::triggered, &(song.undo_stack),
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

  starting_key_slider_pointer->slider_pointer->setValue(
      static_cast<int>(song.starting_key));
  connect(starting_key_slider_pointer->slider_pointer,
          &QAbstractSlider::valueChanged, this,
          &Editor::set_starting_key_with_slider);
  controls_form_pointer->addRow(starting_key_label_pointer,
                                starting_key_slider_pointer);

  starting_volume_slider_pointer->slider_pointer->setValue(
      static_cast<int>(song.starting_volume));
  connect(starting_volume_slider_pointer->slider_pointer,
          &QAbstractSlider::valueChanged, this,
          &Editor::set_starting_volume_with_slider);
  controls_form_pointer->addRow(starting_volume_label_pointer,
                                starting_volume_slider_pointer);

  starting_tempo_slider_pointer->slider_pointer->setValue(
      static_cast<int>(song.starting_tempo));
  connect(starting_tempo_slider_pointer->slider_pointer,
          &QAbstractSlider::valueChanged, this,
          &Editor::set_starting_tempo_with_slider);
  controls_form_pointer->addRow(starting_tempo_label_pointer,
                                starting_tempo_slider_pointer);

  fill_combo_box(*starting_instrument_selector_pointer,
                 song.instrument_pointers, false);
  set_combo_box(*starting_instrument_selector_pointer,
                song.starting_instrument);
  connect(starting_instrument_selector_pointer, &QComboBox::activated, this,
          &Editor::save_starting_instrument);
  controls_form_pointer->addRow(starting_instrument_label_pointer,
                                starting_instrument_selector_pointer);

  controls_widget_pointer->setLayout(controls_form_pointer);

  central_layout_pointer->addWidget(controls_widget_pointer);

  orchestra_editor_pointer->setPlainText(song.orchestra_code);
  orchestra_column_pointer->addWidget(orchestra_editor_pointer);

  connect(save_orchestra_button_pointer, &QAbstractButton::pressed, this,
          &Editor::save_orchestra_text);
  save_orchestra_button_pointer->setFixedWidth(
      save_orchestra_button_pointer->sizeHint().width());
  orchestra_column_pointer->addWidget(save_orchestra_button_pointer);

  orchestra_box_pointer->setLayout(orchestra_column_pointer);

  central_layout_pointer->addWidget(orchestra_box_pointer);

  chords_view_pointer->header()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  chords_view_pointer->setModel(song.chords_model_pointer);
  chords_view_pointer->setItemDelegateForColumn(numerator_column,
                                                numerator_delegate_pointer);
  chords_view_pointer->setItemDelegateForColumn(denominator_column,
                                                denominator_delegate_pointer);
  chords_view_pointer->setItemDelegateForColumn(octave_column,
                                                octave_delegate_pointer);
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

  controls_widget_pointer->setFixedWidth(CONTROLS_WIDTH);

  resize(STARTING_WINDOW_WIDTH, STARTING_WINDOW_HEIGHT);

  setWindowTitle("Justly");
  setCentralWidget(central_widget_pointer);
}

void Editor::copy_selected() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    error_empty("copy");
    return;
  }
  auto first_index = chords_selection[0];
  copy_level =
      song.chords_model_pointer->const_node_from_index(first_index).get_level();
  auto position = first_index.row();
  auto &parent_node = song.chords_model_pointer->node_from_index(
      song.chords_model_pointer->parent(first_index));
  auto &child_pointers = parent_node.child_pointers;
  copied.clear();
  for (int index = position; index < position + chords_selection.size();
       index = index + 1) {
    copied.push_back(
        std::make_unique<TreeNode>(*(child_pointers[index]), &parent_node));
  }
  update_selection_and_actions();
}

void Editor::play_selected() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    error_empty("play");
    return;
  }
  auto first_index = chords_selection[0];
  play(first_index.row(), chords_selection.size(),
       song.chords_model_pointer->parent(first_index));
}

void Editor::save_starting_instrument() {
  auto new_starting_instrument =
      starting_instrument_selector_pointer->currentText();
  if (new_starting_instrument != song.starting_instrument) {
    song.undo_stack.push(new StartingInstrumentChange(
        *this, song.starting_instrument, new_starting_instrument));
  }
}

void Editor::set_starting_instrument(const QString &starting_instrument,
                                     bool should_set_box) {
  song.starting_instrument = starting_instrument;
  if (should_set_box) {
    set_combo_box(*starting_instrument_selector_pointer, starting_instrument);
  }
}

void Editor::insert_before() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    error_empty("insert before");
    return;
  }
  const auto &first_index = chords_selection[0];
  insert(first_index.row(), 1, first_index.parent());
};

void Editor::insert_after() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    error_empty("insert after");
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
    error_empty("paste before");
    return;
  }
  const auto &first_index = chords_selection[0];
  paste(first_index.row(), first_index.parent());
}

void Editor::paste_after() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    error_empty("paste after");
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
  controls_widget_pointer->setVisible(
      view_controls_checkbox_pointer->isChecked());
}

void Editor::view_orchestra() {
  orchestra_box_pointer->setVisible(
      view_orchestra_checkbox_pointer->isChecked());
}

void Editor::view_chords() {
  chords_view_pointer->setVisible(view_chords_checkbox_pointer->isChecked());
}

void Editor::remove_selected() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    error_empty("remove");
    return;
  }
  const auto &first_index = chords_selection[0];
  song.undo_stack.push(new Remove(*(song.chords_model_pointer),
                                  first_index.row(), chords_selection.size(),
                                  first_index.parent()));
  update_selection_and_actions();
}

void Editor::update_selection_and_actions() {
  auto *selection_model_pointer = chords_view_pointer->selectionModel();

  const auto selection = selection_model_pointer->selectedRows();
  const auto parent = chords_view_pointer->currentIndex().parent();

  QItemSelection invalid;

  for (const QModelIndex &index : selection) {
    if (index.parent() != parent) {
      invalid.select(index, index);
    }
  }
  selection_model_pointer->blockSignals(true);
  selection_model_pointer->select(invalid, QItemSelectionModel::Deselect |
                                               QItemSelectionModel::Rows);
  selection_model_pointer->blockSignals(false);

  // revise this later
  auto nothing_selected =
      song.chords_model_pointer->root.get_child_count() == 0;
  auto chords_selection = selection_model_pointer->selectedRows();
  auto any_selected = !(chords_selection.isEmpty());
  auto selected_level = 0;
  auto empty_chord_is_selected = false;
  auto level_match = false;
  if (any_selected) {
    const auto &first_node =
        song.chords_model_pointer->const_node_from_index(chords_selection[0]);
    selected_level = first_node.get_level();
    level_match = selected_level == copy_level;
    empty_chord_is_selected = chords_selection.size() == 1 &&
                              selected_level == chord_level &&
                              first_node.get_child_count() == 0;
  }

  play_selection_action_pointer->setEnabled(any_selected);
  insert_before_action_pointer->setEnabled(any_selected);
  insert_after_action_pointer->setEnabled(any_selected);
  remove_action_pointer->setEnabled(any_selected);
  copy_action_pointer->setEnabled(any_selected);

  paste_before_action_pointer->setEnabled(level_match);
  paste_after_action_pointer->setEnabled(level_match);

  insert_into_action_pointer->setEnabled(nothing_selected ||
                                         empty_chord_is_selected);
  paste_into_action_pointer->setEnabled(
      (nothing_selected && copy_level == chord_level) ||
      (empty_chord_is_selected && copy_level == note_level));
};

auto Editor::set_starting_key_with_slider() -> void {
  if (song.starting_key !=
      starting_key_slider_pointer->slider_pointer->value()) {
    song.undo_stack.push(new StartingKeyChange(
        *this, starting_key_slider_pointer->slider_pointer->value()));
  }
}

auto Editor::set_starting_volume_with_slider() -> void {
  if (song.starting_volume !=
      starting_volume_slider_pointer->slider_pointer->value()) {
    song.undo_stack.push(new StartingVolumeChange(
        *this, starting_volume_slider_pointer->slider_pointer->value()));
  }
}

void Editor::set_starting_tempo_with_slider() {
  if (song.starting_tempo !=
      starting_tempo_slider_pointer->slider_pointer->value()) {
    song.undo_stack.push(new StartingTempoChange(
        *this, starting_tempo_slider_pointer->slider_pointer->value()));
  }
}

void Editor::insert(int position, int rows, const QModelIndex &parent_index) {
  // insertRows will error if invalid
  song.undo_stack.push(new InsertEmptyRows(*(song.chords_model_pointer),
                                           position, rows, parent_index));
};

void Editor::paste(int position, const QModelIndex &parent_index) {
  if (!copied.empty()) {
    song.undo_stack.push(new Insert(*(song.chords_model_pointer), position,
                                    copied, parent_index));
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
    starting_instrument_selector_pointer->clear();
    fill_combo_box(*starting_instrument_selector_pointer,
                   song.instrument_pointers, false);
    set_combo_box(*starting_instrument_selector_pointer,
                  song.starting_instrument);

    starting_key_slider_pointer->slider_pointer->setValue(
        static_cast<int>(song.starting_key));
    starting_volume_slider_pointer->slider_pointer->setValue(
        static_cast<int>(song.starting_volume));
    starting_tempo_slider_pointer->slider_pointer->setValue(
        static_cast<int>(song.starting_tempo));
    orchestra_editor_pointer->setPlainText(song.orchestra_code);
  }
}

void Editor::save_orchestra_text() {
  auto new_orchestra_text = orchestra_editor_pointer->toPlainText();
  if (new_orchestra_text != song.orchestra_code) {
    std::vector<std::unique_ptr<const QString>> new_instrument_pointers;
    extract_instruments(new_instrument_pointers, new_orchestra_text);

    if (new_instrument_pointers.empty()) {
      QMessageBox::warning(nullptr, "Orchestra error",
                           "No instruments. Cannot load");
      return;
    }
    if (!song.chords_model_pointer->verify_instruments(
            new_instrument_pointers)) {
      return;
    }
    stop_playing();
    if (!(song.verify_orchestra_text_compiles(new_orchestra_text))) {
      return;
    }
    auto &old_starting_instrument = song.starting_instrument;
    if (!has_instrument(new_instrument_pointers, song.starting_instrument)) {
      const auto &new_starting_instrument = *(new_instrument_pointers[0]);
      QMessageBox::warning(nullptr, "Orchestra warning",
                           QString("Default instrument %1 no longer exists. "
                                   "Setting default to first instrument %2")
                               .arg(old_starting_instrument)
                               .arg(new_starting_instrument));
      song.undo_stack.push(new OrchestraChange(
          *this, song.orchestra_code, new_orchestra_text,
          old_starting_instrument, new_starting_instrument));
    } else {
      song.undo_stack.push(new OrchestraChange(
          *this, song.orchestra_code, new_orchestra_text,
          old_starting_instrument, old_starting_instrument));
    }
  }
}

void Editor::set_orchestra_text(const QString &new_orchestra_text,
                                const QString &new_starting_instrument,
                                bool should_set_text) {
  song.set_orchestra_text(new_orchestra_text);
  song.starting_instrument = new_starting_instrument;
  starting_instrument_selector_pointer->blockSignals(true);
  starting_instrument_selector_pointer->clear();
  fill_combo_box(*starting_instrument_selector_pointer,
                 song.instrument_pointers, false);
  set_combo_box(*starting_instrument_selector_pointer,
                song.starting_instrument);
  starting_instrument_selector_pointer->blockSignals(false);
  if (should_set_text) {
    orchestra_editor_pointer->setPlainText(new_orchestra_text);
  }
  song.chords_model_pointer->redisplay();
}

void Editor::play(int position, size_t rows, const QModelIndex &parent_index) {
  stop_playing();

  current_key = song.starting_key;
  current_volume = (FULL_NOTE_VOLUME * song.starting_volume) / PERCENT;
  current_tempo = song.starting_tempo;
  current_time = 0.0;
  current_instrument = song.starting_instrument;

  auto end_position = position + rows;
  auto &parent = song.chords_model_pointer->node_from_index(parent_index);
  if (!(parent.verify_child_at(position) &&
        parent.verify_child_at(end_position - 1))) {
    return;
  };
  auto &sibling_pointers = parent.child_pointers;
  auto parent_level = parent.get_level();
  if (parent.is_root()) {
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
  } else if (parent_level == chord_level) {
    auto &grandparent = *(parent.parent_pointer);
    auto &uncle_pointers = grandparent.child_pointers;
    auto parent_position = parent.is_at_row();
    for (auto index = 0; index <= parent_position; index = index + 1) {
      update_with_chord(*uncle_pointers[index]);
    }
    for (auto index = position; index < end_position; index = index + 1) {
      schedule_note(*sibling_pointers[index]);
    }
  } else {
    error_level(parent_level);
  }

  performance_thread.Play();
}

void Editor::update_with_chord(const TreeNode &node) {
  const auto &note_chord_pointer = node.note_chord_pointer;
  current_key = current_key * node.get_ratio();
  current_volume = current_volume * note_chord_pointer->volume_percent / 100.0;
  current_tempo = current_tempo * note_chord_pointer->tempo_percent / 100.0;
  auto chord_instrument = note_chord_pointer->instrument;
  if (chord_instrument != "") {
    current_instrument = chord_instrument;
  }
}

void Editor::schedule_note(const TreeNode &node) {
  auto *note_chord_pointer = node.note_chord_pointer.get();
  auto &instrument = note_chord_pointer->instrument;
  if (instrument == "") {
    instrument = current_instrument;
  }
  performance_thread.InputMessage(qUtf8Printable(
      QString("i \"%1\" %2 %3 %4 %5")
          .arg(instrument)
          .arg(current_time)
          .arg(get_beat_duration() * note_chord_pointer->beats *
               note_chord_pointer->tempo_percent / 100.0)
          .arg(current_key * node.get_ratio()

                   )
          .arg(current_volume * note_chord_pointer->volume_percent / 100.0)));
}

Editor::~Editor() {
  performance_thread.Stop();
  performance_thread.Join();
}

void Editor::stop_playing() {
  performance_thread.Pause();
  performance_thread.FlushMessageQueue();
  csound_session.RewindScore();
}

auto Editor::get_beat_duration() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}
