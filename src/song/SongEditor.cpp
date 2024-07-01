#include "justly/SongEditor.hpp"

#include <qabstractitemdelegate.h>  // for QAbstractItemDelegate
#include <qabstractitemmodel.h>     // for QModelIndex, QAbstra...
#include <qabstractitemview.h>      // for QAbstractItemView
#include <qaction.h>                // for QAction
#include <qassert.h>                // for Q_ASSERT
#include <qbytearray.h>             // for QByteArray
#include <qclipboard.h>             // for QClipboard
#include <qcombobox.h>              // for QComboBox
#include <qdir.h>                   // for QDir
#include <qdockwidget.h>            // for QDockWidget, QDockWi...
#include <qfiledialog.h>            // for QFileDialog, QFileDi...
#include <qformlayout.h>            // for QFormLayout
#include <qframe.h>                 // for QFrame
#include <qguiapplication.h>        // for QGuiApplication
#include <qitemeditorfactory.h>     // for QStandardItemEditorC...
#include <qitemselectionmodel.h>    // for QItemSelectionModel
#include <qkeysequence.h>           // for QKeySequence, QKeySe...
#include <qlineedit.h>              // for QLineEdit
#include <qlist.h>                  // for QList
#include <qmenu.h>                  // for QMenu
#include <qmenubar.h>               // for QMenuBar
#include <qmessagebox.h>            // for QMessageBox, QMessag...
#include <qmetaobject.h>            // for QMetaProperty
#include <qmetatype.h>              // for qMetaTypeId
#include <qmimedata.h>              // for QMimeData
#include <qnamespace.h>             // for ItemDataRole, Horizo...
#include <qobjectdefs.h>            // for QMetaObject
#include <qrect.h>                  // for QRect
#include <qscreen.h>                // for QScreen
#include <qsize.h>                  // for QSize
#include <qsizepolicy.h>            // for QSizePolicy, QSizePo...
#include <qslider.h>                // for QSlider
#include <qspinbox.h>               // for QDoubleSpinBox
#include <qstandardpaths.h>         // for QStandardPaths, QSta...
#include <qstring.h>                // for QString
#include <qstyleoption.h>           // for QStyleOptionViewItem
#include <qundostack.h>             // for QUndoStack
#include <qwidget.h>                // for QWidget

#include <cstddef>                // for size_t
#include <fstream>                // IWYU pragma: keep
#include <initializer_list>       // for initializer_list
#include <iomanip>                // for operator<<, setw
#include <map>                    // for operator!=, operator==
#include <memory>                 // for make_unique, __uniqu...
#include <nlohmann/json.hpp>      // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>  // for json
#include <sstream>                // IWYU pragma: keep
#include <string>                 // for char_traits, string
#include <vector>                 // for vector

#include "cell_editors/InstrumentEditor.hpp"     // for InstrumentEditor
#include "cell_editors/IntervalEditor.hpp"       // for IntervalEditor
#include "cell_editors/RationalEditor.hpp"       // for RationalEditor
#include "changes/InsertRemoveChange.hpp"        // for InsertRemoveChange
#include "changes/StartingInstrumentChange.hpp"  // for StartingInstrumentCh...
#include "changes/StartingKeyChange.hpp"         // for StartingKeyChange
#include "changes/StartingTempoChange.hpp"       // for StartingTempoChange
#include "changes/StartingVolumeChange.hpp"      // for StartingVolumeChange
#include "json/JsonErrorHandler.hpp"             // for show_parse_error
#include "json/schemas.hpp"                      // for verify_json_song
#include "justly/Instrument.hpp"                 // for Instrument (ptr only)
#include "justly/Interval.hpp"                   // for Interval
#include "justly/Rational.hpp"                   // for Rational
#include "justly/Song.hpp"                       // for Song
#include "models/ChordsModel.hpp"                // for ChordsModel, to_pare...
#include "other/ChordsView.hpp"                  // for ChordsView
#include "other/TreeSelector.hpp"                // for TreeSelector
#include "other/private_constants.hpp"           // for PERCENT, MAX_GAIN

auto get_default_driver() -> std::string {
#if defined(__linux__)
  return "pulseaudio";
#elif defined(_WIN32)
  return "wasapi";
#elif defined(__APPLE__)
  return "coreaudio";
#else
  return {};
#endif
}

void SongEditor::update_actions() {
  Q_ASSERT(tree_selector_pointer != nullptr);
  auto selected_row_indexes = tree_selector_pointer->selectedRows();

  auto any_selected = !(selected_row_indexes.isEmpty());
  auto selected_level = root_level;
  auto empty_chord_selected = false;
  if (any_selected) {
    auto &first_index = selected_row_indexes[0];
    selected_level = get_level(first_index);
    Q_ASSERT(chords_model_pointer != nullptr);
    empty_chord_selected = selected_level == chord_level &&
                           selected_row_indexes.size() == 1 &&
                           chords_model_pointer->rowCount(first_index) == 0;
  }
  auto no_chords = song.chords.empty();
  auto can_paste = selected_level != root_level && selected_level == copy_level;

  Q_ASSERT(copy_action_pointer != nullptr);
  copy_action_pointer->setEnabled(any_selected);

  Q_ASSERT(insert_before_action_pointer != nullptr);
  insert_before_action_pointer->setEnabled(any_selected);

  Q_ASSERT(insert_after_action_pointer != nullptr);
  insert_after_action_pointer->setEnabled(any_selected);

  Q_ASSERT(insert_before_action_pointer != nullptr);
  insert_before_action_pointer->setEnabled(any_selected);

  Q_ASSERT(remove_action_pointer != nullptr);
  remove_action_pointer->setEnabled(any_selected);

  Q_ASSERT(play_action_pointer != nullptr);
  play_action_pointer->setEnabled(any_selected);

  Q_ASSERT(insert_into_action_pointer != nullptr);
  insert_into_action_pointer->setEnabled(no_chords || empty_chord_selected);

  Q_ASSERT(paste_before_action_pointer != nullptr);
  paste_before_action_pointer->setEnabled(can_paste);

  Q_ASSERT(paste_after_action_pointer != nullptr);
  paste_after_action_pointer->setEnabled(can_paste);

  Q_ASSERT(paste_into_action_pointer != nullptr);
  paste_into_action_pointer->setEnabled(
      (no_chords && copy_level == chord_level) ||
      (empty_chord_selected && copy_level == note_level));

  Q_ASSERT(save_action_pointer != nullptr);
  Q_ASSERT(undo_stack_pointer != nullptr);
  save_action_pointer->setEnabled(!undo_stack_pointer->isClean() &&
                                  !current_file.empty());
}

void SongEditor::paste(int first_child_number,
                       const QModelIndex &parent_index) {
  auto *clipboard_pointer = QGuiApplication::clipboard();

  Q_ASSERT(clipboard_pointer != nullptr);
  const QMimeData *mime_data_pointer = clipboard_pointer->mimeData();

  Q_ASSERT(mime_data_pointer != nullptr);
  if (mime_data_pointer->hasFormat("application/json")) {
    paste_text(first_child_number,
               mime_data_pointer->data("application/json").toStdString(),
               parent_index);
  }
}

void SongEditor::open() {
  Q_ASSERT(undo_stack_pointer != nullptr);
  if (undo_stack_pointer->isClean() ||
      QMessageBox::question(nullptr, tr("Unsaved changes"),
                            tr("Discard unsaved changes?")) ==
          QMessageBox::Yes) {
    QFileDialog dialog(this, "Open — Justly", current_folder.c_str(),
                       "JSON file (*.json)");

    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setDefaultSuffix(".json");
    dialog.setFileMode(QFileDialog::ExistingFile);

    if (dialog.exec() != 0) {
      current_folder = dialog.directory().absolutePath().toStdString();
      const auto &selected_files = dialog.selectedFiles();
      Q_ASSERT(!(selected_files.empty()));
      open_file(selected_files[0].toStdString());
    }
  }
}

void SongEditor::save_as() {
  QFileDialog dialog(this, "Save As — Justly", current_folder.c_str(),
                     "JSON file (*.json)");

  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setDefaultSuffix(".json");
  dialog.setFileMode(QFileDialog::AnyFile);

  if (dialog.exec() != 0) {
    current_folder = dialog.directory().absolutePath().toStdString();
    const auto &selected_files = dialog.selectedFiles();
    Q_ASSERT(!(selected_files.empty()));
    save_as_file(selected_files[0].toStdString());
  }
}

void SongEditor::export_wav() {
  QFileDialog dialog(this, "Export — Justly", current_folder.c_str(),
                     "WAV file (*.wav)");
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setDefaultSuffix(".wav");
  dialog.setFileMode(QFileDialog::AnyFile);

  dialog.setLabelText(QFileDialog::Accept, "Export");

  if (dialog.exec() != 0) {
    current_folder = dialog.directory().absolutePath().toStdString();
    const auto &selected_files = dialog.selectedFiles();
    Q_ASSERT(!(selected_files.empty()));
    export_to_file(selected_files[0].toStdString());
  }
}

void SongEditor::initialize_controls() {
  Q_ASSERT(starting_key_editor_pointer != nullptr);
  starting_key_editor_pointer->setValue(song.starting_key);

  Q_ASSERT(starting_volume_editor_pointer != nullptr);
  starting_volume_editor_pointer->setValue(song.starting_volume);

  Q_ASSERT(starting_tempo_editor_pointer != nullptr);
  starting_tempo_editor_pointer->setValue(song.starting_tempo);

  Q_ASSERT(starting_instrument_editor_pointer != nullptr);
  starting_instrument_editor_pointer->setValue(
      song.starting_instrument_pointer);
}

SongEditor::SongEditor(QWidget *parent_pointer, Qt::WindowFlags flags)
    : QMainWindow(parent_pointer, flags),
      copy_level(root_level),
      current_folder(
          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
              .toStdString()),
      playback_volume_editor_pointer(new QSlider(Qt::Horizontal, this)),
      starting_instrument_editor_pointer(new InstrumentEditor(this, false)),
      starting_key_editor_pointer(new QDoubleSpinBox(this)),
      starting_volume_editor_pointer(new QDoubleSpinBox(this)),
      starting_tempo_editor_pointer(new QDoubleSpinBox(this)),
      chords_view_pointer(new ChordsView(this)),
      undo_stack_pointer(new QUndoStack(this)),
      insert_before_action_pointer(new QAction(tr("&Before"), this)),
      insert_after_action_pointer(new QAction(tr("&After"), this)),
      insert_into_action_pointer(new QAction(tr("&Into"), this)),
      remove_action_pointer(new QAction(tr("&Remove"), this)),
      copy_action_pointer(new QAction(tr("&Copy"), this)),
      paste_before_action_pointer(new QAction(tr("&Before"), this)),
      paste_after_action_pointer(new QAction(tr("&After"), this)),
      paste_into_action_pointer(new QAction(tr("&Into"), this)),
      save_action_pointer(new QAction(tr("&Save"), this)),
      play_action_pointer(new QAction(tr("&Play selection"), this)) {
  auto *factory = std::make_unique<QItemEditorFactory>().release();
  factory->registerEditor(
      qMetaTypeId<Rational>(),
      std::make_unique<QStandardItemEditorCreator<RationalEditor>>().release());
  factory->registerEditor(
      qMetaTypeId<const Instrument *>(),
      std::make_unique<QStandardItemEditorCreator<InstrumentEditor>>()
          .release());
  factory->registerEditor(
      qMetaTypeId<Interval>(),
      std::make_unique<QStandardItemEditorCreator<IntervalEditor>>().release());
  factory->registerEditor(
      qMetaTypeId<QString>(),
      std::make_unique<QStandardItemEditorCreator<QLineEdit>>().release());
  QItemEditorFactory::setDefaultFactory(factory);

  chords_model_pointer =
      std::make_unique<ChordsModel>(&song, undo_stack_pointer, this).release();

  auto *controls_pointer = std::make_unique<QFrame>(this).release();
  controls_pointer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QDockWidget *dock_widget_pointer =
      std::make_unique<QDockWidget>("Controls", this).release();

  auto *menu_bar_pointer = menuBar();
  Q_ASSERT(menu_bar_pointer != nullptr);

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
          &SongEditor::export_wav);
  file_menu_pointer->addAction(export_as_action_pointer);

  menu_bar_pointer->addMenu(file_menu_pointer);

  auto *edit_menu_pointer =
      std::make_unique<QMenu>(tr("&Edit"), this).release();

  auto *undo_action_pointer =
      undo_stack_pointer->createUndoAction(edit_menu_pointer);
  Q_ASSERT(undo_action_pointer != nullptr);
  undo_action_pointer->setShortcuts(QKeySequence::Undo);
  edit_menu_pointer->addAction(undo_action_pointer);

  auto *redo_action_pointer =
      undo_stack_pointer->createRedoAction(edit_menu_pointer);
  Q_ASSERT(redo_action_pointer != nullptr);
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
  connect(view_controls_checkbox_pointer, &QAction::toggled,
          dock_widget_pointer, &QWidget::setVisible);
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

  playback_volume_editor_pointer->setMinimum(0);
  playback_volume_editor_pointer->setMaximum(PERCENT);
  playback_volume_editor_pointer->setValue(
      static_cast<int>(player.get_playback_volume() / MAX_GAIN * PERCENT));
  connect(playback_volume_editor_pointer, &QSlider::valueChanged, this,
          [this](int new_value) {
            set_playback_volume(
                static_cast<float>(1.0 * new_value / PERCENT * MAX_GAIN));
          });
  controls_form_pointer->addRow(tr("&Playback volume:"),
                                playback_volume_editor_pointer);

  connect(starting_instrument_editor_pointer, &QComboBox::currentIndexChanged,
          this, [this](size_t new_index) {
            const auto &all_instruments = get_all_instruments();
            Q_ASSERT(new_index < all_instruments.size());
            undo_stack_pointer->push(std::make_unique<StartingInstrumentChange>(
                                         this, song.starting_instrument_pointer,
                                         &(all_instruments[new_index]))
                                         .release());
          });
  controls_form_pointer->addRow(tr("Starting &instrument:"),
                                starting_instrument_editor_pointer);

  starting_key_editor_pointer->setMinimum(MIN_STARTING_KEY);
  starting_key_editor_pointer->setMaximum(MAX_STARTING_KEY);
  starting_key_editor_pointer->setDecimals(1);
  starting_key_editor_pointer->setSuffix(" hz");
  connect(starting_key_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](int new_value) {
            undo_stack_pointer->push(std::make_unique<StartingKeyChange>(
                                         this, song.starting_key, new_value)
                                         .release());
          });
  controls_form_pointer->addRow(tr("Starting &key:"),
                                starting_key_editor_pointer);

  starting_volume_editor_pointer->setMinimum(1);
  starting_volume_editor_pointer->setMaximum(MAX_STARTING_VOLUME);
  starting_volume_editor_pointer->setDecimals(1);
  starting_volume_editor_pointer->setSuffix("%");
  connect(starting_volume_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](int new_value) {
            undo_stack_pointer->push(std::make_unique<StartingVolumeChange>(
                                         this, song.starting_volume, new_value)
                                         .release());
          });
  controls_form_pointer->addRow(tr("Starting &volume:"),
                                starting_volume_editor_pointer);

  starting_tempo_editor_pointer->setMinimum(MIN_STARTING_TEMPO);
  starting_tempo_editor_pointer->setMaximum(MAX_STARTING_TEMPO);
  starting_tempo_editor_pointer->setDecimals(1);
  starting_tempo_editor_pointer->setSuffix(" bpm");
  connect(starting_tempo_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](int new_value) {
            undo_stack_pointer->push(std::make_unique<StartingTempoChange>(
                                         this, song.starting_tempo, new_value)
                                         .release());
          });
  controls_form_pointer->addRow(tr("Starting &tempo:"),
                                starting_tempo_editor_pointer);

  controls_pointer->setLayout(controls_form_pointer);

  dock_widget_pointer->setWidget(controls_pointer);
  dock_widget_pointer->setFeatures(QDockWidget::NoDockWidgetFeatures);
  addDockWidget(Qt::LeftDockWidgetArea, dock_widget_pointer);

  tree_selector_pointer =
      std::make_unique<TreeSelector>(chords_model_pointer).release();
  chords_view_pointer->setModel(chords_model_pointer);
  chords_view_pointer->setSelectionModel(tree_selector_pointer);

  connect(tree_selector_pointer, &QItemSelectionModel::selectionChanged, this,
          &SongEditor::update_actions);

  setWindowTitle("Justly");
  setCentralWidget(chords_view_pointer);

  connect(undo_stack_pointer, &QUndoStack::cleanChanged, this,
          &SongEditor::update_actions);

  connect(chords_model_pointer, &QAbstractItemModel::rowsInserted, this,
          &SongEditor::update_actions);
  connect(chords_model_pointer, &QAbstractItemModel::rowsRemoved, this,
          &SongEditor::update_actions);
  connect(chords_model_pointer, &QAbstractItemModel::modelReset, this,
          &SongEditor::update_actions);

  const auto *primary_screen_pointer = QGuiApplication::primaryScreen();
  Q_ASSERT(primary_screen_pointer != nullptr);
  resize(sizeHint().width(),
         primary_screen_pointer->availableGeometry().height());

  initialize_controls();
  undo_stack_pointer->clear();
  undo_stack_pointer->setClean();
}

SongEditor::~SongEditor() {
  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->disconnect();
}

auto SongEditor::get_current_file() const -> const std::string & {
  return current_file;
}

auto SongEditor::get_selected_rows() const -> QModelIndexList {
  Q_ASSERT(tree_selector_pointer != nullptr);
  return tree_selector_pointer->selectedRows();
}

auto SongEditor::get_index(int parent_number, int child_number,
                           NoteChordField column_number) const -> QModelIndex {
  Q_ASSERT(chords_model_pointer != nullptr);
  return chords_model_pointer->get_index(parent_number, child_number,
                                         column_number);
}

void SongEditor::select_index(const QModelIndex index) {
  Q_ASSERT(tree_selector_pointer != nullptr);
  tree_selector_pointer->select(
      index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

void SongEditor::select_indices(const QModelIndex first_index,
                                const QModelIndex last_index) {
  Q_ASSERT(tree_selector_pointer != nullptr);
  tree_selector_pointer->select(
      QItemSelection(first_index, last_index),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

void SongEditor::clear_selection() {
  Q_ASSERT(tree_selector_pointer != nullptr);
  tree_selector_pointer->select(QModelIndex(), QItemSelectionModel::Clear);
}

auto SongEditor::get_number_of_children(int parent_index) -> size_t {
  return song.get_number_of_children(parent_index);
}

auto SongEditor::get_header_data(NoteChordField column_number,
                                 Qt::Orientation orientation,
                                 Qt::ItemDataRole role) const -> QVariant {
  Q_ASSERT(chords_model_pointer != nullptr);
  return chords_model_pointer->headerData(column_number, orientation, role);
}

auto SongEditor::get_row_count(QModelIndex parent_index) const -> int {
  Q_ASSERT(chords_model_pointer != nullptr);
  return chords_model_pointer->rowCount(parent_index);
};
auto SongEditor::get_column_count(QModelIndex parent_index) const -> int {
  Q_ASSERT(chords_model_pointer != nullptr);
  return chords_model_pointer->columnCount(parent_index);
};

auto SongEditor::get_parent_index(QModelIndex index) const -> QModelIndex {
  Q_ASSERT(chords_model_pointer != nullptr);
  return chords_model_pointer->parent(index);
};

auto SongEditor::get_flags(QModelIndex index) const -> Qt::ItemFlags {
  Q_ASSERT(chords_model_pointer != nullptr);
  return chords_model_pointer->flags(index);
}

auto SongEditor::get_data(QModelIndex index, Qt::ItemDataRole role)
    -> QVariant {
  Q_ASSERT(chords_model_pointer != nullptr);
  return chords_model_pointer->data(index, role);
};

auto SongEditor::set_data(QModelIndex index, const QVariant &new_value,
                          Qt::ItemDataRole role) -> bool {
  Q_ASSERT(chords_model_pointer != nullptr);
  return chords_model_pointer->setData(index, new_value, role);
};

auto SongEditor::create_editor(QModelIndex index) -> QWidget * {
  Q_ASSERT(chords_model_pointer != nullptr);

  auto *my_delegate_pointer = chords_view_pointer->itemDelegate();
  Q_ASSERT(my_delegate_pointer != nullptr);

  auto *cell_editor_pointer = my_delegate_pointer->createEditor(
      chords_view_pointer->viewport(), QStyleOptionViewItem(), index);
  Q_ASSERT(cell_editor_pointer != nullptr);

  my_delegate_pointer->setEditorData(cell_editor_pointer, index);

  return cell_editor_pointer;
}

void SongEditor::set_editor(QWidget *cell_editor_pointer, QModelIndex index,
                            const QVariant &new_value) {
  Q_ASSERT(chords_view_pointer != nullptr);
  auto *my_delegate_pointer = chords_view_pointer->itemDelegate();

  Q_ASSERT(cell_editor_pointer != nullptr);
  const auto *cell_editor_meta_object = cell_editor_pointer->metaObject();

  Q_ASSERT(cell_editor_meta_object != nullptr);
  cell_editor_pointer->setProperty(
      cell_editor_meta_object->userProperty().name(), new_value);

  Q_ASSERT(chords_model_pointer != nullptr);
  my_delegate_pointer->setModelData(cell_editor_pointer, chords_model_pointer,
                                    index);
}

auto SongEditor::get_playback_volume() const -> float {
  return player.get_playback_volume();
};

void SongEditor::set_playback_volume(float new_playback_volume) {
  player.set_playback_volume(new_playback_volume);
}

auto SongEditor::get_starting_instrument() const -> const Instrument * {
  return song.starting_instrument_pointer;
}

void SongEditor::set_starting_instrument_directly(const Instrument *new_value) {
  Q_ASSERT(starting_instrument_editor_pointer != nullptr);
  if (starting_instrument_editor_pointer->value() != new_value) {
    starting_instrument_editor_pointer->blockSignals(true);
    starting_instrument_editor_pointer->setValue(new_value);
    starting_instrument_editor_pointer->blockSignals(false);
  }
  song.starting_instrument_pointer = new_value;
}

void SongEditor::set_starting_instrument_undoable(const Instrument *new_value) {
  Q_ASSERT(starting_instrument_editor_pointer != nullptr);
  if (starting_instrument_editor_pointer->value() != new_value) {
    starting_instrument_editor_pointer->setValue(new_value);
  }
}

auto SongEditor::get_starting_key() const -> double {
  return song.starting_key;
}

void SongEditor::set_starting_key_directly(double new_value) {
  Q_ASSERT(starting_key_editor_pointer != nullptr);
  if (starting_key_editor_pointer->value() != new_value) {
    starting_key_editor_pointer->blockSignals(true);
    starting_key_editor_pointer->setValue(new_value);
    starting_key_editor_pointer->blockSignals(false);
  }
  song.starting_key = new_value;
}

void SongEditor::set_starting_key_undoable(double new_value) {
  Q_ASSERT(starting_key_editor_pointer != nullptr);
  if (starting_key_editor_pointer->value() != new_value) {
    starting_key_editor_pointer->setValue(new_value);
  }
}

auto SongEditor::get_starting_volume() const -> double {
  return song.starting_volume;
};

void SongEditor::set_starting_volume_directly(double new_value) {
  Q_ASSERT(starting_volume_editor_pointer != nullptr);
  if (starting_volume_editor_pointer->value() != new_value) {
    starting_volume_editor_pointer->blockSignals(true);
    starting_volume_editor_pointer->setValue(new_value);
    starting_volume_editor_pointer->blockSignals(false);
  }
  song.starting_volume = new_value;
}

void SongEditor::set_starting_volume_undoable(double new_value) {
  Q_ASSERT(starting_volume_editor_pointer != nullptr);
  if (starting_volume_editor_pointer->value() != new_value) {
    starting_volume_editor_pointer->setValue(new_value);
  }
}

auto SongEditor::get_starting_tempo() const -> double {
  return song.starting_tempo;
};

void SongEditor::set_starting_tempo_directly(double new_value) {
  Q_ASSERT(starting_tempo_editor_pointer != nullptr);
  if (starting_tempo_editor_pointer->value() != new_value) {
    starting_tempo_editor_pointer->blockSignals(true);
    starting_tempo_editor_pointer->setValue(new_value);
    starting_tempo_editor_pointer->blockSignals(false);
  }
  song.starting_tempo = new_value;
}

void SongEditor::set_starting_tempo_undoable(double new_value) {
  Q_ASSERT(starting_tempo_editor_pointer != nullptr);
  if (starting_tempo_editor_pointer->value() != new_value) {
    starting_tempo_editor_pointer->setValue(new_value);
  }
}

void SongEditor::undo() {
  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->undo();
}

void SongEditor::redo() {
  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->redo();
}

void SongEditor::copy_selected() {
  auto selected_row_indexes = get_selected_rows();
  Q_ASSERT(!(selected_row_indexes.empty()));
  auto first_index = selected_row_indexes[0];

  Q_ASSERT(chords_model_pointer != nullptr);
  auto parent_index = chords_model_pointer->parent(first_index);
  copy_level = get_level(first_index);
  auto *new_data_pointer = std::make_unique<QMimeData>().release();
  std::stringstream json_text;

  json_text << std::setw(4)
            << chords_model_pointer->copy(first_index.row(),
                                          selected_row_indexes.size(),
                                          to_parent_index(parent_index));

  Q_ASSERT(new_data_pointer != nullptr);
  new_data_pointer->setData("application/json",
                            QByteArray::fromStdString(json_text.str()));

  auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  clipboard_pointer->setMimeData(new_data_pointer);
  update_actions();
}

void SongEditor::paste_text(int first_child_number, const std::string &text,
                            const QModelIndex &parent_index) {
  nlohmann::json json_song;
  try {
    json_song = nlohmann::json::parse(text);
  } catch (const nlohmann::json::parse_error &parse_error) {
    show_parse_error(parse_error.what());
    return;
  }

  if (!verify_children(get_level(parent_index), json_song)) {
    return;
  }

  Q_ASSERT(undo_stack_pointer != nullptr);
  Q_ASSERT(chords_model_pointer != nullptr);
  undo_stack_pointer->push(std::make_unique<InsertRemoveChange>(
                               chords_model_pointer, first_child_number,
                               json_song, to_parent_index(parent_index), true)
                               .release());
}

void SongEditor::paste_before() {
  auto selected_row_indexes = get_selected_rows();
  Q_ASSERT(!selected_row_indexes.empty());
  const auto &first_index = selected_row_indexes[0];
  paste(first_index.row(), first_index.parent());
}

void SongEditor::paste_after() {
  auto selected_row_indexes = get_selected_rows();
  Q_ASSERT(!selected_row_indexes.empty());
  const auto &last_index =
      selected_row_indexes[selected_row_indexes.size() - 1];
  paste(last_index.row() + 1, last_index.parent());
}

void SongEditor::paste_into() {
  auto selected_row_indexes = get_selected_rows();
  paste(0,
        selected_row_indexes.empty() ? QModelIndex() : selected_row_indexes[0]);
}

void SongEditor::insert_before() {
  auto selected_row_indexes = get_selected_rows();
  Q_ASSERT(!selected_row_indexes.empty());
  const auto &first_index = selected_row_indexes[0];
  chords_model_pointer->insertRows(first_index.row(), 1, first_index.parent());
}

void SongEditor::insert_after() {
  auto selected_row_indexes = get_selected_rows();
  Q_ASSERT(!selected_row_indexes.empty());
  const auto &last_index =
      selected_row_indexes[selected_row_indexes.size() - 1];
  chords_model_pointer->insertRows(last_index.row() + 1, 1,
                                   last_index.parent());
}

void SongEditor::insert_into() {
  auto selected_row_indexes = get_selected_rows();
  chords_model_pointer->insertRows(
      0, 1,
      selected_row_indexes.empty() ? QModelIndex() : selected_row_indexes[0]);
}

void SongEditor::remove_selected() {
  auto selected_row_indexes = get_selected_rows();
  Q_ASSERT(!selected_row_indexes.empty());
  const auto &first_index = selected_row_indexes[0];
  chords_model_pointer->removeRows(
      first_index.row(), static_cast<int>(selected_row_indexes.size()),
      first_index.parent());
}

void SongEditor::open_file(const std::string &filename) {
  try {
    std::ifstream file_io(filename.c_str());
    auto json_song = nlohmann::json::parse(file_io);
    file_io.close();
    if (verify_json_song(json_song)) {
      song.load_starting_values(json_song);
      initialize_controls();

      Q_ASSERT(chords_model_pointer != nullptr);
      chords_model_pointer->load_chords(json_song);

      Q_ASSERT(undo_stack_pointer != nullptr);
      undo_stack_pointer->clear();
      undo_stack_pointer->setClean();
    }
  } catch (const nlohmann::json::parse_error &parse_error) {
    show_parse_error(parse_error.what());
    return;
  }
}

void SongEditor::save() { save_as_file(get_current_file()); }

void SongEditor::save_as_file(const std::string &filename) {
  std::ofstream file_io(filename.c_str());
  file_io << std::setw(4) << song.json();
  file_io.close();
  current_file = filename;

  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->setClean();
}

void SongEditor::start_real_time() { player.start_real_time(); }

void SongEditor::export_to_file(const std::string &output_file) {
  player.export_to_file(output_file);
}

void SongEditor::play_selected() {
  auto selected_row_indexes = get_selected_rows();

  Q_ASSERT(!(selected_row_indexes.empty()));
  auto first_index = selected_row_indexes[0];
  Q_ASSERT(chords_model_pointer != nullptr);
  player.play(to_parent_index(chords_model_pointer->parent(first_index)),
              first_index.row(), selected_row_indexes.size());
}

void SongEditor::stop_playing() { player.stop_playing(); }
