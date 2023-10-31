#include "justly/editors/SongEditor.h"

#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <qabstractitemmodel.h>    // for QModelIndex
#include <qaction.h>               // for QAction
#include <qboxlayout.h>            // for QVBoxLayout
#include <qbytearray.h>            // for QByteArray
#include <qclipboard.h>            // for QClipboard
#include <qcombobox.h>             // for QComboBox
#include <qcontainerfwd.h>         // for QStringList
#include <qdir.h>
#include <qfiledialog.h>          // for QFileDialog, QFileDialog::...
#include <qframe.h>                            // for QFrame, QFrame::Styled...
#include <qformlayout.h>          // for QFormLayout
#include <qguiapplication.h>      // for QGuiApplication
#include <qitemselectionmodel.h>  // for QItemSelectionModel, QItem...
#include <qkeysequence.h>         // for QKeySequence, QKeySequence...
#include <qlist.h>                // for QList, QList<>::const_iter...
#include <qmenu.h>                // for QMenu
#include <qmenubar.h>             // for QMenuBar
#include <qmessagebox.h>          // for QMessageBox, QMessageBox::Yes
#include <qmimedata.h>            // for QMimeData
#include <qnamespace.h>           // for WindowFlags
#include <qrect.h>
#include <qscreen.h>
#include <qsize.h>
#include <qsizepolicy.h>
#include <qstring.h>   // for QString
#include <qvariant.h>  // for QVariant, operator!=
#include <qwidget.h>   // for QWidget

#include <fstream>
#include <initializer_list>       // for initializer_list
#include <map>                    // for operator!=, operator==
#include <memory>                 // for make_unique, __unique_ptr_t
#include <nlohmann/json.hpp>      // for basic_json, basic_json<>::...
#include <nlohmann/json_fwd.hpp>  // for json
#include <vector>                 // for vector

#include "justly/main/Player.h"                // for Player
#include "justly/main/Song.h"                  // for Song
#include "justly/metatypes/Instrument.h"       // for Instrument
#include "justly/models/ChordsModel.h"         // for ChordsModel
#include "justly/notechord/Chord.h"            // for Chord
#include "src/commands/StartingValueChange.h"  // for StartingValueChange
#include "src/main/ChordsView.h"
#include "src/utilities/JsonErrorHandler.h"

SongEditor::SongEditor(QWidget *parent_pointer, Qt::WindowFlags flags)
    : QMainWindow(parent_pointer, flags),
      chords_view_pointer(new ChordsView(this)) {
  auto *central_widget_pointer = std::make_unique<QFrame>(this).release();

  auto *controls_pointer =
      std::make_unique<QFrame>(central_widget_pointer).release();
  controls_pointer->setFrameStyle(QFrame::StyledPanel);
  controls_pointer->setAutoFillBackground(true);
  controls_pointer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  auto *menu_bar_pointer = menuBar();

  auto *file_menu_pointer =
      std::make_unique<QMenu>(tr("&File"), this).release();

  auto *open_action_pointer =
      std::make_unique<QAction>(tr("&Open"), file_menu_pointer).release();
  file_menu_pointer->addAction(open_action_pointer);
  connect(open_action_pointer, &QAction::triggered, this, &SongEditor::open);
  open_action_pointer->setShortcuts(QKeySequence::Open);

  file_menu_pointer->addSeparator();

  save_action_pointer->setShortcuts(QKeySequence::Save);
  connect(save_action_pointer, &QAction::triggered, this, &SongEditor::save);
  file_menu_pointer->addAction(save_action_pointer);
  save_action_pointer->setEnabled(false);

  auto *save_as_action_pointer =
      std::make_unique<QAction>(tr("&Save As..."), file_menu_pointer).release();
  save_as_action_pointer->setShortcuts(QKeySequence::SaveAs);
  connect(save_as_action_pointer, &QAction::triggered, this,
          &SongEditor::save_as);
  file_menu_pointer->addAction(save_as_action_pointer);
  save_as_action_pointer->setEnabled(true);

  auto *export_as_action_pointer =
      std::make_unique<QAction>(tr("&Export recording"), file_menu_pointer)
          .release();
  connect(export_as_action_pointer, &QAction::triggered, this,
          &SongEditor::export_recording);
  file_menu_pointer->addAction(export_as_action_pointer);

  menu_bar_pointer->addMenu(file_menu_pointer);

  auto *edit_menu_pointer =
      std::make_unique<QMenu>(tr("&Edit"), this).release();

  auto *undo_action_pointer =
      undo_stack_pointer->createUndoAction(edit_menu_pointer);
  undo_action_pointer->setShortcuts(QKeySequence::Undo);
  edit_menu_pointer->addAction(undo_action_pointer);

  auto *redo_action_pointer =
      undo_stack_pointer->createRedoAction(edit_menu_pointer);
  redo_action_pointer->setShortcuts(QKeySequence::Redo);
  edit_menu_pointer->addAction(redo_action_pointer);

  edit_menu_pointer->addSeparator();

  copy_action_pointer->setEnabled(false);
  copy_action_pointer->setShortcuts(QKeySequence::Copy);
  connect(copy_action_pointer, &QAction::triggered, this,
          &SongEditor::copy_selected);
  edit_menu_pointer->addAction(copy_action_pointer);

  auto *paste_menu_pointer =
      std::make_unique<QMenu>(tr("&Paste"), edit_menu_pointer).release();

  paste_before_action_pointer->setEnabled(false);
  connect(paste_before_action_pointer, &QAction::triggered, this,
          &SongEditor::paste_before);
  paste_menu_pointer->addAction(paste_before_action_pointer);

  paste_after_action_pointer->setEnabled(false);
  paste_after_action_pointer->setShortcuts(QKeySequence::Paste);
  connect(paste_after_action_pointer, &QAction::triggered, this,
          &SongEditor::paste_after);
  paste_menu_pointer->addAction(paste_after_action_pointer);

  paste_into_action_pointer->setEnabled(false);
  connect(paste_into_action_pointer, &QAction::triggered, this,
          &SongEditor::paste_into);
  paste_menu_pointer->addAction(paste_into_action_pointer);

  edit_menu_pointer->addMenu(paste_menu_pointer);

  edit_menu_pointer->addSeparator();

  auto *insert_menu_pointer =
      std::make_unique<QMenu>(tr("&Insert"), edit_menu_pointer).release();

  edit_menu_pointer->addMenu(insert_menu_pointer);

  insert_before_action_pointer->setEnabled(false);
  connect(insert_before_action_pointer, &QAction::triggered, this,
          &SongEditor::insert_before);
  insert_menu_pointer->addAction(insert_before_action_pointer);

  insert_after_action_pointer->setEnabled(false);
  insert_after_action_pointer->setShortcuts(QKeySequence::InsertLineSeparator);
  connect(insert_after_action_pointer, &QAction::triggered, this,
          &SongEditor::insert_after);
  insert_menu_pointer->addAction(insert_after_action_pointer);

  insert_into_action_pointer->setEnabled(true);
  insert_into_action_pointer->setShortcuts(QKeySequence::AddTab);
  connect(insert_into_action_pointer, &QAction::triggered, this,
          &SongEditor::insert_into);
  insert_menu_pointer->addAction(insert_into_action_pointer);

  remove_action_pointer->setEnabled(false);
  remove_action_pointer->setShortcuts(QKeySequence::Delete);
  connect(remove_action_pointer, &QAction::triggered, this,
          &SongEditor::remove_selected);
  edit_menu_pointer->addAction(remove_action_pointer);

  menu_bar_pointer->addMenu(edit_menu_pointer);

  auto *view_menu_pointer =
      std::make_unique<QMenu>(tr("&View"), this).release();

  auto *view_controls_checkbox_pointer =
      std::make_unique<QAction>(tr("&Controls"), view_menu_pointer).release();

  view_controls_checkbox_pointer->setCheckable(true);
  view_controls_checkbox_pointer->setChecked(true);
  connect(view_controls_checkbox_pointer, &QAction::toggled, controls_pointer,
          &QWidget::setVisible);
  view_menu_pointer->addAction(view_controls_checkbox_pointer);

  menu_bar_pointer->addMenu(view_menu_pointer);

  auto *play_menu_pointer =
      std::make_unique<QMenu>(tr("&Play"), this).release();

  play_action_pointer->setEnabled(false);
  play_action_pointer->setShortcuts(QKeySequence::Print);
  connect(play_action_pointer, &QAction::triggered, this,
          &SongEditor::play_selected);
  play_menu_pointer->addAction(play_action_pointer);

  auto *stop_playing_action_pointer =
      std::make_unique<QAction>(tr("&Stop playing"), play_menu_pointer)
          .release();
  stop_playing_action_pointer->setEnabled(true);
  play_menu_pointer->addAction(stop_playing_action_pointer);
  connect(stop_playing_action_pointer, &QAction::triggered, this,
          &SongEditor::stop_playing);
  stop_playing_action_pointer->setShortcuts(QKeySequence::Cancel);

  menu_bar_pointer->addMenu(play_menu_pointer);

  auto *controls_form_pointer =
      std::make_unique<QFormLayout>(controls_pointer).release();

  starting_key_editor_pointer->setMinimum(MINIMUM_STARTING_KEY);
  starting_key_editor_pointer->setMaximum(MAXIMUM_STARTING_KEY);
  starting_key_editor_pointer->setDecimals(1);
  starting_key_editor_pointer->setSuffix(" hz");
  connect(starting_key_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          &SongEditor::save_starting_key);
  controls_form_pointer->addRow(tr("Starting &key:"),
                                starting_key_editor_pointer);

  starting_volume_editor_pointer->setMinimum(MINIMUM_STARTING_VOLUME);
  starting_volume_editor_pointer->setMaximum(MAXIMUM_STARTING_VOLUME);
  starting_volume_editor_pointer->setDecimals(1);
  starting_volume_editor_pointer->setSuffix("%");
  connect(starting_volume_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          &SongEditor::save_starting_volume);
  controls_form_pointer->addRow(tr("Starting &volume:"),
                                starting_volume_editor_pointer);

  starting_tempo_editor_pointer->setMinimum(MINIMUM_STARTING_TEMPO);
  starting_tempo_editor_pointer->setMaximum(MAXIMUM_STARTING_TEMPO);
  starting_tempo_editor_pointer->setDecimals(1);
  starting_tempo_editor_pointer->setSuffix(" bpm");
  connect(starting_tempo_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          &SongEditor::save_starting_tempo);
  controls_form_pointer->addRow(tr("Starting &tempo:"),
                                starting_tempo_editor_pointer);

  initialize_controls();

  connect(starting_instrument_editor_pointer, &QComboBox::currentIndexChanged,
          this, &SongEditor::save_starting_instrument);
  controls_form_pointer->addRow(tr("Starting &instrument:"),
                                starting_instrument_editor_pointer);

  controls_pointer->setLayout(controls_form_pointer);

  auto *central_layout_pointer =
      std::make_unique<QVBoxLayout>(central_widget_pointer).release();

  central_layout_pointer->addWidget(controls_pointer);

  chords_view_pointer->setModel(chords_model_pointer);
  connect(chords_view_pointer->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &SongEditor::fix_selection);
  chords_view_pointer->setSizePolicy(QSizePolicy::MinimumExpanding,
                                     QSizePolicy::Expanding);

  central_layout_pointer->addWidget(chords_view_pointer);

  central_widget_pointer->setLayout(central_layout_pointer);

  setWindowTitle("Justly");
  setCentralWidget(central_widget_pointer);

  connect(undo_stack_pointer, &QUndoStack::cleanChanged, this,
          &SongEditor::update_actions);

  connect(chords_model_pointer, &QAbstractItemModel::rowsInserted, this,
          &SongEditor::update_actions);
  connect(chords_model_pointer, &QAbstractItemModel::rowsRemoved, this,
          &SongEditor::update_actions);
  connect(chords_model_pointer, &QAbstractItemModel::modelReset, this,
          &SongEditor::update_actions);
  resize(sizeHint().width(),
         QGuiApplication::primaryScreen()->availableGeometry().height());
}

void SongEditor::copy_selected() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  auto first_index = chords_selection[0];
  auto parent_index = chords_model_pointer->parent(first_index);
  copy_level = ChordsModel::get_level(first_index);
  auto *new_data_pointer = std::make_unique<QMimeData>().release();
  new_data_pointer->setData(
      "application/json",
      QByteArray::fromStdString(
          chords_model_pointer
              ->copyJsonChildren(first_index.row(),
                                 static_cast<int>(chords_selection.size()),
                                 parent_index)
              .dump()));
  QGuiApplication::clipboard()->setMimeData(new_data_pointer);
  update_actions();
}

SongEditor::~SongEditor() { undo_stack_pointer->disconnect(); }

void SongEditor::play_selected() const {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  auto first_index = chords_selection[0];
  play(first_index.row(), static_cast<int>(chords_selection.size()),
       chords_model_pointer->parent(first_index));
}

void SongEditor::save_starting_value(StartingFieldId value_type,
                                     const QVariant &new_value) {
  undo_stack_pointer->push(
      std::make_unique<StartingValueChange>(
          this, value_type, get_starting_value(value_type), new_value)
          .release());
}

void SongEditor::insert_before() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &first_index = chords_selection[0];
  insert(first_index.row(), 1, first_index.parent());
}

void SongEditor::insert_after() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &last_index = chords_selection[chords_selection.size() - 1];
  insert(last_index.row() + 1, 1, last_index.parent());
}

void SongEditor::insert_into() {
  auto chords_selection = get_selected_rows();
  insert(0, 1, chords_selection.empty() ? QModelIndex() : chords_selection[0]);
}

void SongEditor::paste_before() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &first_index = chords_selection[0];
  paste(first_index.row(), first_index.parent());
}

void SongEditor::paste_after() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &last_index = chords_selection[chords_selection.size() - 1];
  paste(last_index.row() + 1, last_index.parent());
}

void SongEditor::paste_into() {
  auto chords_selection = get_selected_rows();
  paste(0, chords_selection.empty() ? QModelIndex() : chords_selection[0]);
}

void SongEditor::remove_selected() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &first_index = chords_selection[0];
  chords_model_pointer->removeRows(first_index.row(),
                                   static_cast<int>(chords_selection.size()),
                                   first_index.parent());
}

void SongEditor::fix_selection(const QItemSelection &selected,
                               const QItemSelection & /*deselected*/) {
  auto *selection_model_pointer = chords_view_pointer->selectionModel();
  auto all_initial_rows = selection_model_pointer->selectedRows();

  if (!all_initial_rows.isEmpty()) {
    // find an item that was already selected
    auto holdover = selection_model_pointer->selection();
    holdover.merge(selected, QItemSelectionModel::Deselect);
    // if there was holdovers, use the previous parent
    // if not, use the parent of the first new item
    const QModelIndex current_parent_index =
        holdover.isEmpty() ? all_initial_rows[0].parent()
                           : holdover[0].topLeft().parent();

    QItemSelection invalid;

    for (const QModelIndex &index : all_initial_rows) {
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
  }

  update_actions();
}

void SongEditor::update_actions() {
  auto chords_selection = chords_view_pointer->selectionModel()->selectedRows();
  auto any_selected = !(chords_selection.isEmpty());
  auto selected_level = root_level;
  auto empty_item_selected = false;
  if (any_selected) {
    auto &first_index = chords_selection[0];
    selected_level = ChordsModel::get_level(first_index);
    empty_item_selected = chords_selection.size() == 1 &&
                          chords_model_pointer->rowCount(first_index) == 0;
  }
  auto no_chords = song.chord_pointers.empty();
  auto can_paste = selected_level != root_level && selected_level == copy_level;

  copy_action_pointer->setEnabled(any_selected);
  insert_before_action_pointer->setEnabled(any_selected);
  insert_after_action_pointer->setEnabled(any_selected);
  insert_before_action_pointer->setEnabled(any_selected);
  remove_action_pointer->setEnabled(any_selected);
  play_action_pointer->setEnabled(any_selected);
  insert_into_action_pointer->setEnabled(
      no_chords || (selected_level == chord_level && empty_item_selected));

  paste_before_action_pointer->setEnabled(can_paste);
  paste_after_action_pointer->setEnabled(can_paste);
  paste_into_action_pointer->setEnabled(
      (no_chords && copy_level == chord_level) ||
      (selected_level == chord_level && empty_item_selected &&
       copy_level == note_level));

  save_action_pointer->setEnabled(!undo_stack_pointer->isClean() &&
                                  !current_file.isEmpty());
}

void SongEditor::save_starting_key(int new_value) {
  undo_stack_pointer->push(
      std::make_unique<StartingValueChange>(
          this, starting_key_id, get_starting_value(starting_key_id), new_value)
          .release());
}

void SongEditor::save_starting_volume(int new_value) {
  save_starting_value(starting_volume_id, QVariant::fromValue(new_value));
}

void SongEditor::save_starting_tempo(int new_value) {
  save_starting_value(starting_tempo_id, QVariant::fromValue(new_value));
}

void SongEditor::save_starting_instrument(int new_index) {
  save_starting_value(
      starting_instrument_id,
      QVariant::fromValue(&(Instrument::get_all_instruments().at(new_index))));
}

void SongEditor::insert(int first_child_number, int number_of_children,
                        const QModelIndex &parent_index) {
  chords_model_pointer->insertRows(first_child_number, number_of_children,
                                   parent_index);
}

void SongEditor::paste(int first_child_number,
                       const QModelIndex &parent_index) {
  const QMimeData *mime_data_pointer = QGuiApplication::clipboard()->mimeData();
  if (mime_data_pointer->hasFormat("application/json")) {
    paste_text(first_child_number,
               mime_data_pointer->data("application/json").toStdString(),
               parent_index);
  }
}

void SongEditor::save() { save_as_file(get_current_file()); }

void SongEditor::save_as() {
  QFileDialog dialog(this);

  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setDefaultSuffix(".json");
  dialog.setDirectory(current_folder);
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setNameFilter("JSON file (*.json)");

  if (dialog.exec() != 0) {
    current_folder = dialog.directory().absolutePath();
    save_as_file(dialog.selectedFiles()[0]);
  }
}

void SongEditor::save_as_file(const QString &filename) {
  std::ofstream file_io(qUtf8Printable(filename));
  file_io << song.to_json();
  file_io.close();
  current_file = filename;
  undo_stack_pointer->setClean();
}

void SongEditor::export_recording() {
  QFileDialog dialog(this);
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setDefaultSuffix(".wav");
  dialog.setDirectory(current_folder);
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setNameFilter("WAV file (*.wav)");

  dialog.setLabelText(QFileDialog::Accept, "Export");

  if (dialog.exec() != 0) {
    current_folder = dialog.directory().absolutePath();
    export_recording_to(dialog.selectedFiles()[0]);
  }
}

void SongEditor::export_recording_to(const QString &filename) {
  player_pointer = std::make_unique<Player>(&song, filename.toStdString());
  player_pointer->write_song();
  player_pointer = std::make_unique<Player>(&song);
}

void SongEditor::open() {
  if (undo_stack_pointer->isClean() ||
      QMessageBox::question(nullptr, tr("Unsaved changes"),
                            tr("Discard unsaved changes?")) ==
          QMessageBox::Yes) {
    QFileDialog dialog(this);

    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setDefaultSuffix(".json");
    dialog.setDirectory(current_folder);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter("JSON file (*.json)");

    if (dialog.exec() != 0) {
      current_folder = dialog.directory().absolutePath();
      open_file(dialog.selectedFiles()[0]);
    }
  }
}

void SongEditor::initialize_controls() {
  set_starting_control(starting_instrument_id,
                       get_starting_value(starting_instrument_id), true);
  set_starting_control(starting_key_id, get_starting_value(starting_key_id),
                       true);
  set_starting_control(starting_volume_id,
                       get_starting_value(starting_volume_id), true);
  set_starting_control(starting_tempo_id, get_starting_value(starting_tempo_id),
                       true);
}

void SongEditor::open_file(const QString &filename) {
  try {
    std::ifstream file_io(qUtf8Printable(filename));
    auto json_song = nlohmann::json::parse(file_io);
    file_io.close();
    if (Song::verify_json(json_song)) {
      chords_model_pointer->begin_reset_model();
      song.load_from(json_song);
      chords_model_pointer->end_reset_model();
      initialize_controls();
      undo_stack_pointer->resetClean();
    }
  } catch (const nlohmann::json::parse_error &parse_error) {
    JsonErrorHandler::show_parse_error(parse_error.what());
    return;
  }
}

void SongEditor::paste_text(int first_child_number, const std::string &text,
                            const QModelIndex &parent_index) {
  nlohmann::json json_song;
  try {
    json_song = nlohmann::json::parse(text);
  } catch (const nlohmann::json::parse_error &parse_error) {
    JsonErrorHandler::show_parse_error(parse_error.what());
    return;
  }

  if (!ChordsModel::verify_json_children(parent_index, json_song)) {
    return;
  }
  chords_model_pointer->insertJsonChildren(first_child_number, json_song,
                                           parent_index);
}

void SongEditor::play(int first_child_number, int number_of_children,
                      const QModelIndex &parent_index) const {
  player_pointer->write_chords(
      first_child_number, number_of_children,
      chords_model_pointer->get_chord_number(parent_index));
}

void SongEditor::stop_playing() const { player_pointer->stop_playing(); }

auto SongEditor::has_real_time() const -> bool {
  return player_pointer->has_real_time();
}

void SongEditor::undo() { undo_stack_pointer->undo(); }

void SongEditor::redo() { undo_stack_pointer->redo(); }

auto SongEditor::get_current_file() const -> const QString & {
  return current_file;
}

void SongEditor::set_starting_control(StartingFieldId value_type,
                                      const QVariant &new_value,
                                      bool no_signals) {
  switch (value_type) {
    case starting_key_id: {
      auto new_double = new_value.toDouble();
      if (starting_key_editor_pointer->value() != new_double) {
        if (no_signals) {
          starting_key_editor_pointer->blockSignals(true);
        }
        starting_key_editor_pointer->setValue(new_double);
        if (no_signals) {
          starting_key_editor_pointer->blockSignals(false);
        }
      }
      song.starting_key = new_double;
      break;
    }
    case starting_volume_id: {
      auto new_double = new_value.toDouble();
      if (starting_volume_editor_pointer->value() != new_double) {
        if (no_signals) {
          starting_volume_editor_pointer->blockSignals(true);
        }
        starting_volume_editor_pointer->setValue(new_double);
        if (no_signals) {
          starting_volume_editor_pointer->blockSignals(false);
        }
      }
      song.starting_volume = new_double;
      break;
    }
    case starting_tempo_id: {
      auto new_double = new_value.toDouble();
      if (starting_tempo_editor_pointer->value() != new_double) {
        if (no_signals) {
          starting_tempo_editor_pointer->blockSignals(true);
        }
        starting_tempo_editor_pointer->setValue(new_double);
        if (no_signals) {
          starting_tempo_editor_pointer->blockSignals(false);
        }
      }
      song.starting_tempo = new_double;
      break;
    }
    default:  // starting_instrument_id
      if (starting_instrument_editor_pointer->get_instrument_pointer() !=
          new_value.value<const Instrument *>()) {
        if (no_signals) {
          starting_instrument_editor_pointer->blockSignals(true);
        }
        starting_instrument_editor_pointer->set_instrument_pointer(
            new_value.value<const Instrument *>());
        if (no_signals) {
          starting_instrument_editor_pointer->blockSignals(false);
        }
      }
      song.starting_instrument_pointer = new_value.value<const Instrument *>();
      break;
  }
}

auto SongEditor::get_chords_model_pointer() const -> QAbstractItemModel * {
  return chords_model_pointer;
}

auto SongEditor::get_chords_view_pointer() const -> QAbstractItemView * {
  return chords_view_pointer;
}

auto SongEditor::get_selected_rows() const -> QModelIndexList {
  return chords_view_pointer->selectionModel()->selectedRows();
}

auto SongEditor::get_starting_value(StartingFieldId value_type) const
    -> QVariant {
  switch (value_type) {
    case starting_key_id:
      return QVariant::fromValue(song.starting_key);
    case starting_volume_id:
      return QVariant::fromValue(song.starting_volume);
    case starting_tempo_id:
      return QVariant::fromValue(song.starting_tempo);
    default:  // starting_instrument_id
      return QVariant::fromValue(song.starting_instrument_pointer);
  }
}

auto SongEditor::get_number_of_children(int chord_number) const -> int {
  const auto &chord_pointers = song.chord_pointers;
  if (chord_number == -1) {
    return static_cast<int>(chord_pointers.size());
  }
  return static_cast<int>(chord_pointers[chord_number]->note_pointers.size());
};
