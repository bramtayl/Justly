#include "main/Editor.h"

#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <qabstractitemmodel.h>    // for QModelIndex
#include <qaction.h>               // for QAction
#include <qboxlayout.h>            // for QVBoxLayout
#include <qbytearray.h>            // for QByteArray
#include <qclipboard.h>            // for QClipboard
#include <qcombobox.h>             // for QComboBox
#include <qcontainerfwd.h>         // for QStringList
#include <qdir.h>
#include <qfiledialog.h>      // for QFileDialog, QFileDialog::...
#include <qformlayout.h>      // for QFormLayout
#include <qguiapplication.h>  // for QGuiApplication
#include <qitemeditorfactory.h>
#include <qitemselectionmodel.h>  // for QItemSelectionModel, QItem...
#include <qkeysequence.h>         // for QKeySequence, QKeySequence...
#include <qlabel.h>               // for QLabel
#include <qlineedit.h>
#include <qlist.h>        // for QList, QList<>::const_iter...
#include <qmenu.h>        // for QMenu
#include <qmenubar.h>     // for QMenuBar
#include <qmessagebox.h>  // for QMessageBox, QMessageBox::Yes
#include <qmetaobject.h>
#include <qmetatype.h>
#include <qmimedata.h>   // for QMimeData
#include <qnamespace.h>  // for WindowFlags
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

#include "commands/StartingValueChange.h"  // for StartingValueChange
#include "editors/IntervalEditor.h"
#include "main/Player.h"           // for Player
#include "main/Song.h"             // for Song
#include "metatypes/Instrument.h"  // for Instrument
#include "metatypes/Interval.h"
#include "models/ChordsModel.h"   // for ChordsModel
#include "notechord/Chord.h"      // for Chord
#include "notechord/NoteChord.h"  // for chord_level, note_level
#include "utilities/JsonErrorHandler.h"

Editor::Editor(QWidget *parent_pointer, Qt::WindowFlags flags)
    : QMainWindow(parent_pointer, flags) {
  auto factory = gsl::not_null(new QItemEditorFactory());

  factory->registerEditor(
      QMetaType::fromType<const Instrument *>().id(),
      gsl::not_null(new QStandardItemEditorCreator<InstrumentEditor>()));
  factory->registerEditor(
      QMetaType::fromType<Interval>().id(),
      gsl::not_null(new QStandardItemEditorCreator<IntervalEditor>()));
  factory->registerEditor(
      QMetaType::Int,
      gsl::not_null(new QStandardItemEditorCreator<QSpinBox>()));
  factory->registerEditor(
      QMetaType::Double,
      gsl::not_null(new QStandardItemEditorCreator<QDoubleSpinBox>()));
  factory->registerEditor(
      QMetaType::QString,
      gsl::not_null(new QStandardItemEditorCreator<QLineEdit>()));

  QItemEditorFactory::setDefaultFactory(factory);

  auto central_widget_pointer = gsl::not_null(new QWidget(this));

  auto controls_pointer = gsl::not_null(new QWidget(central_widget_pointer));

  auto menu_bar_pointer = gsl::not_null(menuBar());

  auto file_menu_pointer = gsl::not_null(new QMenu(tr("&File"), this));

  auto open_action_pointer =
      gsl::not_null(new QAction(tr("&Open"), file_menu_pointer));
  file_menu_pointer->addAction(open_action_pointer);
  connect(open_action_pointer, &QAction::triggered, this, &Editor::open);
  open_action_pointer->setShortcuts(QKeySequence::Open);

  file_menu_pointer->addSeparator();

  save_action_pointer->setShortcuts(QKeySequence::Save);
  connect(save_action_pointer, &QAction::triggered, this, &Editor::save);
  file_menu_pointer->addAction(save_action_pointer);
  save_action_pointer->setEnabled(false);

  auto save_as_action_pointer =
      gsl::not_null(new QAction(tr("&Save As..."), file_menu_pointer));
  save_as_action_pointer->setShortcuts(QKeySequence::SaveAs);
  connect(save_as_action_pointer, &QAction::triggered, this, &Editor::save_as);
  file_menu_pointer->addAction(save_as_action_pointer);
  save_as_action_pointer->setEnabled(true);

  auto export_as_action_pointer =
      gsl::not_null(new QAction(tr("&Export recording"), file_menu_pointer));
  connect(export_as_action_pointer, &QAction::triggered, this,
          &Editor::export_recording);
  file_menu_pointer->addAction(export_as_action_pointer);

  menu_bar_pointer->addMenu(file_menu_pointer);

  auto edit_menu_pointer = gsl::not_null(new QMenu(tr("&Edit"), this));

  auto undo_action_pointer =
      gsl::not_null(undo_stack_pointer->createUndoAction(edit_menu_pointer));
  undo_action_pointer->setShortcuts(QKeySequence::Undo);
  edit_menu_pointer->addAction(undo_action_pointer);

  auto redo_action_pointer =
      gsl::not_null(undo_stack_pointer->createRedoAction(edit_menu_pointer));
  redo_action_pointer->setShortcuts(QKeySequence::Redo);
  edit_menu_pointer->addAction(redo_action_pointer);

  edit_menu_pointer->addSeparator();

  copy_action_pointer->setEnabled(false);
  copy_action_pointer->setShortcuts(QKeySequence::Copy);
  connect(copy_action_pointer, &QAction::triggered, this,
          &Editor::copy_selected);
  edit_menu_pointer->addAction(copy_action_pointer);

  auto paste_menu_pointer =
      gsl::not_null(new QMenu(tr("&Paste"), edit_menu_pointer));

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

  auto insert_menu_pointer =
      gsl::not_null(new QMenu(tr("&Insert"), edit_menu_pointer));

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

  auto view_menu_pointer = gsl::not_null(new QMenu(tr("&View"), this));

  auto view_controls_checkbox_pointer =
      gsl::not_null(new QAction(tr("&Controls"), view_menu_pointer));

  view_controls_checkbox_pointer->setCheckable(true);
  view_controls_checkbox_pointer->setChecked(true);
  connect(view_controls_checkbox_pointer, &QAction::toggled, controls_pointer,
          &QWidget::setVisible);
  view_menu_pointer->addAction(view_controls_checkbox_pointer);

  menu_bar_pointer->addMenu(view_menu_pointer);

  auto play_menu_pointer = gsl::not_null(new QMenu(tr("&Play"), this));

  play_action_pointer->setEnabled(false);
  play_action_pointer->setShortcuts(QKeySequence::Print);
  connect(play_action_pointer, &QAction::triggered, this,
          &Editor::play_selected);
  play_menu_pointer->addAction(play_action_pointer);

  auto stop_playing_action_pointer =
      gsl::not_null(new QAction(tr("&Stop playing"), play_menu_pointer));
  stop_playing_action_pointer->setEnabled(true);
  play_menu_pointer->addAction(stop_playing_action_pointer);
  connect(stop_playing_action_pointer, &QAction::triggered, this,
          &Editor::stop_playing);
  stop_playing_action_pointer->setShortcuts(QKeySequence::Cancel);

  menu_bar_pointer->addMenu(play_menu_pointer);

  auto controls_form_pointer = gsl::not_null(new QFormLayout(controls_pointer));

  starting_key_editor_pointer->setSizePolicy(QSizePolicy::Fixed,
                                             QSizePolicy::Fixed);
  starting_key_editor_pointer->setMinimum(MINIMUM_STARTING_KEY);
  starting_key_editor_pointer->setMaximum(MAXIMUM_STARTING_KEY);
  starting_key_editor_pointer->setDecimals(1);
  starting_key_editor_pointer->setSuffix(" hz");
  connect(starting_key_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          &Editor::save_starting_key);
  controls_form_pointer->addRow(
      gsl::not_null(new QLabel(tr("Starting key"), controls_pointer)),
      starting_key_editor_pointer);

  starting_volume_editor_pointer->setMinimum(MINIMUM_STARTING_VOLUME);
  starting_volume_editor_pointer->setMaximum(MAXIMUM_STARTING_VOLUME);
  starting_volume_editor_pointer->setDecimals(1);
  starting_volume_editor_pointer->setSuffix("%");
  starting_volume_editor_pointer->setSizePolicy(QSizePolicy::Fixed,
                                                QSizePolicy::Fixed);
  connect(starting_volume_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          &Editor::save_starting_volume);
  controls_form_pointer->addRow(
      gsl::not_null(new QLabel(tr("Starting volume"), controls_pointer)),
      starting_volume_editor_pointer);

  starting_tempo_editor_pointer->setMinimum(MINIMUM_STARTING_TEMPO);
  starting_tempo_editor_pointer->setMaximum(MAXIMUM_STARTING_TEMPO);
  starting_tempo_editor_pointer->setDecimals(1);
  starting_tempo_editor_pointer->setSuffix(" bpm");
  starting_tempo_editor_pointer->setSizePolicy(QSizePolicy::Fixed,
                                               QSizePolicy::Fixed);
  connect(starting_tempo_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          &Editor::save_starting_tempo);
  controls_form_pointer->addRow(
      gsl::not_null(new QLabel(tr("Starting tempo"), controls_pointer)),
      starting_tempo_editor_pointer);

  initialize_controls();

  starting_instrument_editor_pointer->setSizePolicy(QSizePolicy::Fixed,
                                                    QSizePolicy::Fixed);
  connect(starting_instrument_editor_pointer, &QComboBox::currentIndexChanged,
          this, &Editor::save_starting_instrument);
  controls_form_pointer->addRow(
      gsl::not_null(new QLabel(tr("Starting instrument"), controls_pointer)),
      starting_instrument_editor_pointer);

  controls_pointer->setLayout(controls_form_pointer);

  auto central_layout_pointer =
      gsl::not_null(new QVBoxLayout(central_widget_pointer));

  central_layout_pointer->addWidget(controls_pointer);

  chords_view_pointer->setModel(chords_model_pointer);
  chords_view_pointer->setItemDelegate(my_delegate_pointer);
  connect(chords_view_pointer->selectionModel(),
          &QItemSelectionModel::selectionChanged, this, &Editor::fix_selection);

  central_layout_pointer->addWidget(chords_view_pointer);

  central_widget_pointer->setLayout(central_layout_pointer);

  setWindowTitle("Justly");
  setCentralWidget(central_widget_pointer);

  connect(undo_stack_pointer, &QUndoStack::cleanChanged, this,
          &Editor::update_actions);

  connect(chords_model_pointer, &QAbstractItemModel::rowsInserted, this,
          &Editor::update_actions);
  connect(chords_model_pointer, &QAbstractItemModel::rowsRemoved, this,
          &Editor::update_actions);
  connect(chords_model_pointer, &QAbstractItemModel::modelReset, this,
          &Editor::update_actions);
  resize(sizeHint().width(),
         QGuiApplication::primaryScreen()->availableGeometry().height());
}

void Editor::copy_selected() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  auto first_index = chords_selection[0];
  auto parent_index = chords_model_pointer->parent(first_index);
  copy_level = ChordsModel::get_level(first_index);
  auto new_data_pointer = gsl::not_null(new QMimeData());
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

Editor::~Editor() { undo_stack_pointer->disconnect(); }

void Editor::play_selected() const {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  auto first_index = chords_selection[0];
  play(first_index.row(), static_cast<int>(chords_selection.size()),
       chords_model_pointer->parent(first_index));
}

void Editor::save_starting_value(StartingFieldId value_type,
                                 const QVariant &new_value) {
  const auto &old_value = song_pointer->get_starting_value(value_type);
  if (old_value != new_value) {
    set_starting_value(value_type, new_value);
    undo_stack_pointer->push(gsl::not_null(
        new StartingValueChange(this, value_type, old_value, new_value)));
  }
}

void Editor::insert_before() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &first_index = chords_selection[0];
  insert(first_index.row(), 1, first_index.parent());
}

void Editor::insert_after() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &last_index = chords_selection[chords_selection.size() - 1];
  insert(last_index.row() + 1, 1, last_index.parent());
}

void Editor::insert_into() {
  auto chords_selection = get_selected_rows();
  insert(0, 1, chords_selection.empty() ? QModelIndex() : chords_selection[0]);
}

void Editor::paste_before() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &first_index = chords_selection[0];
  paste(first_index.row(), first_index.parent());
}

void Editor::paste_after() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &last_index = chords_selection[chords_selection.size() - 1];
  paste(last_index.row() + 1, last_index.parent());
}

void Editor::paste_into() {
  auto chords_selection = get_selected_rows();
  paste(0, chords_selection.empty() ? QModelIndex() : chords_selection[0]);
}

void Editor::remove_selected() {
  auto chords_selection = get_selected_rows();
  if (chords_selection.empty()) {
    return;
  }
  const auto &first_index = chords_selection[0];
  chords_model_pointer->removeRows(first_index.row(),
                                   static_cast<int>(chords_selection.size()),
                                   first_index.parent());
}

void Editor::fix_selection(const QItemSelection &selected,
                           const QItemSelection & /*deselected*/) {
  auto *selection_model_pointer = chords_view_pointer->selectionModel();
  auto all_initial_rows = selection_model_pointer->selectedRows();

  if (!all_initial_rows.isEmpty()) {
    // find an item that was already selected
    auto holdover = selection_model_pointer->selection();
    holdover.merge(selected, QItemSelectionModel::Deselect);
    // if there was holdovers, use the previous parent
    // if not, use the parent of the first new item
    QModelIndex current_parent_index = holdover.isEmpty()
                                           ? all_initial_rows[0].parent()
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

void Editor::update_actions() {
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
  auto no_chords = song_pointer->chord_pointers.empty();
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

void Editor::save_starting_key(int new_value) {
  save_starting_value(starting_key_id, QVariant::fromValue(new_value));
}

void Editor::save_starting_volume(int new_value) {
  save_starting_value(starting_volume_id, QVariant::fromValue(new_value));
}

void Editor::save_starting_tempo(int new_value) {
  save_starting_value(starting_tempo_id, QVariant::fromValue(new_value));
}

void Editor::save_starting_instrument(int new_index) {
  save_starting_value(
      starting_instrument_id,
      QVariant::fromValue(&(Instrument::get_all_instruments().at(new_index))));
}

void Editor::insert(int first_child_number, int number_of_children,
                    const QModelIndex &parent_index) {
  chords_model_pointer->insertRows(first_child_number, number_of_children,
                                   parent_index);
}

void Editor::paste(int first_child_number, const QModelIndex &parent_index) {
  const QMimeData *mime_data_pointer = QGuiApplication::clipboard()->mimeData();
  if (mime_data_pointer->hasFormat("application/json")) {
    paste_text(first_child_number,
               mime_data_pointer->data("application/json").toStdString(),
               parent_index);
  }
}

void Editor::save() { save_as_file(get_current_file()); }

void Editor::save_as() {
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

void Editor::save_as_file(const QString &filename) {
  std::ofstream file_io(qUtf8Printable(filename));
  file_io << song_pointer->to_json();
  file_io.close();
  current_file = filename;
  undo_stack_pointer->setClean();
}

void Editor::export_recording() {
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

void Editor::export_recording_to(const QString &filename) {
  player_pointer = std::make_unique<Player>(song_pointer.get(), filename);
  player_pointer->write_song();
  player_pointer = std::make_unique<Player>(song_pointer.get());
}

void Editor::open() {
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

void Editor::initialize_controls() const {
  initialize_control(starting_instrument_id);
  initialize_control(starting_key_id);
  initialize_control(starting_volume_id);
  initialize_control(starting_tempo_id);
}

void Editor::open_file(const QString &filename) {
  try {
    std::ifstream file_io(qUtf8Printable(filename));
    auto json_song = nlohmann::json::parse(file_io);
    file_io.close();
    if (Song::verify_json(json_song)) {
      chords_model_pointer->begin_reset_model();
      song_pointer->load_from(json_song);
      chords_model_pointer->end_reset_model();
      initialize_controls();
      undo_stack_pointer->resetClean();
    }
  } catch (const nlohmann::json::parse_error &parse_error) {
    JsonErrorHandler::show_parse_error(parse_error.what());
    return;
  }
}

void Editor::paste_text(int first_child_number, const std::string &text,
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

void Editor::play(int first_child_number, int number_of_children,
                  const QModelIndex &parent_index) const {
  player_pointer->write_chords(
      first_child_number, number_of_children,
      chords_model_pointer->get_chord_number(parent_index));
}

void Editor::stop_playing() const { player_pointer->stop_playing(); }

auto Editor::has_real_time() const -> bool {
  return player_pointer->has_real_time();
}

void Editor::undo() { undo_stack_pointer->undo(); }

void Editor::redo() { undo_stack_pointer->redo(); }

auto Editor::get_current_file() const -> const QString & {
  return current_file;
}

auto Editor::get_control_value(StartingFieldId value_type) const -> QVariant {
  switch (value_type) {
    case starting_key_id:
      return QVariant::fromValue(starting_key_editor_pointer->value());
    case starting_volume_id:
      return QVariant::fromValue(starting_volume_editor_pointer->value());
    case starting_tempo_id:
      return QVariant::fromValue(starting_tempo_editor_pointer->value());
    default:  // starting_instrument_id
      return QVariant::fromValue(starting_instrument_editor_pointer->value());
  }
}

void Editor::set_control(StartingFieldId value_type,
                         const QVariant &new_value) const {
  if (get_control_value(value_type) != new_value) {
    switch (value_type) {
      case starting_key_id:
        starting_key_editor_pointer->setValue(new_value.toDouble());
        return;
      case starting_volume_id:
        starting_volume_editor_pointer->setValue(new_value.toDouble());
        return;
      case starting_tempo_id:
        starting_tempo_editor_pointer->setValue(new_value.toDouble());
        return;
      default:  // starting_instrument_id:
        starting_instrument_editor_pointer->setValue(
            new_value.value<const Instrument *>());
    }
  }
}

void Editor::starting_block_signal(StartingFieldId value_type,
                                   bool should_block) const {
  switch (value_type) {
    case starting_key_id:
      starting_key_editor_pointer->blockSignals(should_block);
      break;
    case starting_volume_id:
      starting_volume_editor_pointer->blockSignals(should_block);
      break;
    case starting_tempo_id:
      starting_tempo_editor_pointer->blockSignals(should_block);
      break;
    default:  // starting_instrument_id
      starting_instrument_editor_pointer->blockSignals(should_block);
  }
}

void Editor::set_control_no_signals(StartingFieldId value_type,
                                    const QVariant &new_value) const {
  if (get_control_value(value_type) != new_value) {
    starting_block_signal(value_type, true);
    set_control(value_type, new_value);
    starting_block_signal(value_type, false);
    set_starting_value(value_type, new_value);
  }
}

void Editor::initialize_control(StartingFieldId value_type) const {
  set_control_no_signals(value_type,
                         song_pointer->get_starting_value(value_type));
}

auto Editor::get_selector_pointer() -> gsl::not_null<QItemSelectionModel *> {
  return chords_view_pointer->selectionModel();
}

auto Editor::get_chords_model_pointer() const -> gsl::not_null<ChordsModel *> {
  return chords_model_pointer;
}

auto Editor::get_delegate_pointer() const -> gsl::not_null<const MyDelegate *> {
  return my_delegate_pointer;
}

auto Editor::get_viewport_pointer() const -> gsl::not_null<QWidget *> {
  return chords_view_pointer->viewport();
}

auto Editor::get_selected_rows() const -> QModelIndexList {
  return chords_view_pointer->selectionModel()->selectedRows();
}

void Editor::set_starting_value(StartingFieldId value_type,
                                const QVariant &new_value) const {
  song_pointer->set_starting_value(value_type, new_value);
}

auto Editor::get_starting_value(StartingFieldId value_type) const -> QVariant {
  return song_pointer->get_starting_value(value_type);
}

auto Editor::get_number_of_children(int chord_number) const -> int {
  const auto &chord_pointers = song_pointer->chord_pointers;
  if (chord_number == -1) {
    return static_cast<int>(chord_pointers.size());
  }
  return static_cast<int>(chord_pointers[chord_number]->note_pointers.size());
};
