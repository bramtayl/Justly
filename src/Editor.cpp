#include "Editor.h"

#include <QtCore/qglobal.h>        // for qCritical
#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <qabstractitemmodel.h>    // for QModelIndex
#include <qabstractitemview.h>     // for QAbstractItemView, QAbstractItem...
#include <qabstractslider.h>       // for QAbstractSlider
#include <qbytearray.h>            // for QByteArray
#include <qclipboard.h>
#include <qcoreapplication.h>  // for QCoreApplication
#include <qdir.h>              // for QDir
#include <qfile.h>             // for QFile
#include <qfiledialog.h>       // for QFileDialog
#include <qguiapplication.h>
#include <qheaderview.h>          // for QHeaderView, QHeaderView::Resize...
#include <qiodevice.h>            // for QIODevice
#include <qiodevicebase.h>        // for QIODeviceBase::ReadOnly, QIODevi...
#include <qitemselectionmodel.h>  // for QItemSelectionModel, QItemSelection
#include <qjsondocument.h>        // for QJsonDocument
#include <qkeysequence.h>         // for QKeySequence, QKeySequence::AddTab
#include <qlabel.h>               // for QLabel
#include <qlist.h>                // for QList, QList<>::const_iterator
#include <qmenubar.h>             // for QMenuBar
#include <qmetatype.h>            // for QMetaType
#include <qmimedata.h>
#include <qslider.h>         // for QSlider
#include <qstandardpaths.h>  // for QStandardPaths, QStandardPaths::...
#include <qundostack.h>      // for QUndoStack

#include <csound/csPerfThread.hpp>  // for CsoundPerformanceThread
#include <csound/csound.hpp>        // for Csound
#include <utility>                  // for move

#include "ChordsModel.h"         // for ChordsModel
#include "ComboBoxDelegate.h"    // for ComboBoxDelegate, MAX_COMBO_BOX_...
#include "Instrument.h"          // for Instrument
#include "Interval.h"            // for Interval
#include "IntervalDelegate.h"    // for IntervalDelegate
#include "NoteChord.h"           // for NoteChord, chord_level, error_level
#include "ShowSlider.h"          // for ShowSlider
#include "ShowSliderDelegate.h"  // for ShowSliderDelegate
#include "Song.h"                // for Song, FULL_NOTE_VOLUME, SECONDS_...
#include "SpinBoxDelegate.h"     // for SpinBoxDelegate
#include "SuffixedNumber.h"      // for SuffixedNumber
#include "TreeNode.h"            // for TreeNode
#include "commands.h"            // for Insert, InsertEmptyRows, Remove
#include "utilities.h"           // for error_empty, set_combo_box, cann...

Editor::Editor(const QString &starting_instrument_input,
               QWidget *parent_pointer, Qt::WindowFlags flags)
    : song(Song(starting_instrument_input)),
      clipboard_pointer(QGuiApplication::clipboard()),
      orchestra_code(get_orchestra_code(song.instruments)),
      QMainWindow(parent_pointer, flags) {
  start_csound();

  QMetaType::registerConverter<Interval, QString>(&Interval::get_text);
  QMetaType::registerConverter<SuffixedNumber, QString>(
      &SuffixedNumber::get_text);

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
  set_combo_box(*starting_instrument_selector_pointer,
                song.starting_instrument);
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

auto get_orchestra_code(const std::vector<Instrument> &instruments) -> QString {
  auto orchestra_code =
      QString(R"(nchnls = 2
0dbfs = 1

gisound_font sfload "%1"
; because 0dbfs = 1, not 32767, I guess
gibase_amplitude = 1/32767
; velocity is how hard you hit the key (not how loud it is)
gimax_velocity = 127
; short release
girelease_duration = 0.05

; arguments p1 = instrument, p2 = start_time, p3 = duration, p4 = instrument_number, p5 = frequency, p6 = amplitude (max 1)
instr play_soundfont
  ; assume velociy is proportional to amplitude
  ; arguments velocity, midi number, amplitude, frequency, preset number, ignore midi flag
  aleft_sound, aright_sound sfplay3 gimax_velocity * p7, p6, gibase_amplitude * p7, p5, p4, 1
  ; arguments start_level, sustain_duration, mid_level, release_duration, end_level
  acutoff_envelope linsegr 1, p3, 1, girelease_duration, 0
  ; cutoff instruments at end of the duration
  aleft_sound_cut = aleft_sound * acutoff_envelope
  aright_sound_cut = aright_sound * acutoff_envelope
  outs aleft_sound_cut, aright_sound_cut
endin
)")
          .arg(QDir(QCoreApplication::applicationDirPath())
                   .filePath("../share/MuseScore_General.sf2"));

  for (int index = 0; index < instruments.size(); index = index + 1) {
    const auto &instrument = instruments[index];
    orchestra_code =
        orchestra_code + QString("gi%1 sfpreset %2, %3, gisound_font, %4\n")
                             .arg(instrument.code)
                             .arg(instrument.preset_number)
                             .arg(instrument.bank_number)
                             .arg(instrument.id);
  }
  return orchestra_code;
}

void Editor::start_csound() {
  csound_session.SetOption("--output=devaudio");
  // csound_session.SetOption("--messagelevel=16");  // comment this out to debug csound
  auto orchestra_error_code =
      csound_session.CompileOrc(qUtf8Printable(orchestra_code));
  if (orchestra_error_code != 0) {
    qCritical("Cannot compile orchestra, error code %d", orchestra_error_code);
    return;
  }

  csound_session.Start();
}

void Editor::copy_selected() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  if (chords_selection.empty()) {
    error_empty("copy");
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
    error_empty("play");
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
    set_combo_box(*starting_instrument_selector_pointer,
                  new_starting_instrument);
    starting_instrument_selector_pointer->blockSignals(false);
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
  controls_pointer->setVisible(view_controls_checkbox_pointer->isChecked());
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
  undo_stack.push(
      std::make_unique<Remove>(*(chords_model_pointer), first_index.row(),
                               chords_selection.size(), first_index.parent())
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
  auto no_chords = song.root.get_child_count() == 0;
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
                              first_node.get_child_count() == 0;
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
  undo_stack.push(
      std::make_unique<InsertEmptyRows>(*(chords_model_pointer), first_index,
                                        number_of_children, parent_index)
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
    undo_stack.resetClean();
    QFile input(filename);
    if (input.open(QIODevice::ReadOnly)) {
      load_text(input.readAll());
      input.close();
    } else {
      cannot_open_error(filename);
    }
  }
}

void Editor::paste_text(int first_index, const QByteArray &paste_text,
                        const QModelIndex &parent_index) {
  const QJsonDocument document = QJsonDocument::fromJson(paste_text);
  if (!verify_json_document(document)) {
    return;
  }
  if (!(document.isArray())) {
    json_parse_error("Expected JSON array!");
    return;
  }
  const auto json_array = document.array();
  if (!chords_model_pointer->verify_json_children(song, json_array,
                                                  parent_index)) {
    return;
  }
  undo_stack.push(std::make_unique<Insert>(*(chords_model_pointer), first_index,
                                           json_array, parent_index)
                      .release());
}

void Editor::load_text(const QByteArray &song_text) {
  chords_model_pointer->begin_reset_model();
  if (song.load_text(song_text)) {
    set_combo_box(*starting_instrument_selector_pointer,
                  song.starting_instrument);

    starting_key_show_slider_pointer->slider_pointer->setValue(
        static_cast<int>(song.starting_key));
    starting_volume_show_slider_pointer->slider_pointer->setValue(
        static_cast<int>(song.starting_volume));
    starting_tempo_show_slider_pointer->slider_pointer->setValue(
        static_cast<int>(song.starting_tempo));
  }
  chords_model_pointer->end_reset_model();
}

void Editor::play(int first_index, int number_of_children,
                  const QModelIndex &parent_index) {
  stop_playing();

  current_key = song.starting_key;
  current_volume = (FULL_NOTE_VOLUME * song.starting_volume) / PERCENT;
  current_tempo = song.starting_tempo;
  current_time = 0.0;
  current_instrument_id = song.get_instrument_id(song.starting_instrument);

  auto end_position = first_index + number_of_children;
  auto &parent_node = chords_model_pointer->get_node(parent_index);
  if (!(parent_node.verify_child_at(first_index) &&
        parent_node.verify_child_at(end_position - 1))) {
    return;
  };
  auto parent_level = parent_node.get_level();
  if (parent_level == root_level) {
    for (auto chord_index = 0; chord_index < first_index;
         chord_index = chord_index + 1) {
      update_with_chord(*parent_node.child_pointers[chord_index]);
    }
    for (auto chord_index = first_index; chord_index < end_position;
         chord_index = chord_index + 1) {
      auto &chord = *parent_node.child_pointers[chord_index];
      update_with_chord(chord);
      for (const auto &note_node_pointer : chord.child_pointers) {
        schedule_note(*note_node_pointer);
      }
      current_time =
          current_time + get_beat_duration() * chord.note_chord_pointer->beats;
    }
  } else if (parent_level == chord_level) {
    auto &root = *(parent_node.parent_pointer);
    auto &chord_pointers = root.child_pointers;
    auto chord_position = parent_node.is_at_row();
    for (auto chord_index = 0; chord_index <= chord_position;
         chord_index = chord_index + 1) {
      update_with_chord(*chord_pointers[chord_index]);
    }
    for (auto note_index = first_index; note_index < end_position;
         note_index = note_index + 1) {
      schedule_note(*parent_node.child_pointers[note_index]);
    }
  } else {
    error_level(parent_level);
  }

  performance_thread_pointer->Play();
}

void Editor::update_with_chord(const TreeNode &node) {
  const auto &note_chord_pointer = node.note_chord_pointer;
  current_key = current_key * node.get_ratio();
  current_volume = current_volume * note_chord_pointer->volume_percent / 100.0;
  current_tempo = current_tempo * note_chord_pointer->tempo_percent / 100.0;
  auto maybe_chord_instrument_name = note_chord_pointer->instrument;
  if (maybe_chord_instrument_name != "") {
    current_instrument_id = song.get_instrument_id(maybe_chord_instrument_name);
  }
}

void Editor::schedule_note(const TreeNode &node) {
  auto *note_chord_pointer = node.note_chord_pointer.get();
  auto maybe_instrument_name = note_chord_pointer->instrument;
  int instrument_id = current_instrument_id;
  if (maybe_instrument_name != "") {
    instrument_id = song.get_instrument_id(maybe_instrument_name);
  }
  auto frequency = current_key * node.get_ratio();
  performance_thread_pointer->InputMessage(qUtf8Printable(
      QString("i \"play_soundfont\" %1 %2 %3 %4 %5 %6")
          .arg(current_time)
          .arg(get_beat_duration() * note_chord_pointer->beats *
               note_chord_pointer->tempo_percent / 100.0)
          .arg(instrument_id)
          .arg(frequency)
          .arg(12 * log2(frequency / 440) + 69)
          .arg(current_volume * note_chord_pointer->volume_percent / 100.0)));
}

Editor::~Editor() {
  if (performance_thread_pointer->GetStatus() == 0) {
    performance_thread_pointer->Stop();
    performance_thread_pointer->Join();
  }
}

void Editor::stop_playing() {
  // 0 if still playing
  if (performance_thread_pointer->GetStatus() == 0) {
    performance_thread_pointer->Stop();
    performance_thread_pointer->Join();
    csound_session.Stop();
    csound_session.Reset();
    start_csound();
    performance_thread_pointer =
        std::move(std::make_unique<CsoundPerformanceThread>(&csound_session));
  }
}

auto Editor::get_beat_duration() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}
