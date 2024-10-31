#include "song/SongEditor.hpp"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QAction>
#include <QBoxLayout>
#include <QByteArray>
#include <QClipboard>
#include <QCloseEvent>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGuiApplication>
#include <QHeaderView>
#include <QItemEditorFactory>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaType>
#include <QMimeData>
#include <QRect>
#include <QScreen>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QString>
#include <QTableView>
#include <QTextStream>
#include <QUndoStack>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
#include <algorithm>
#include <exception>
#include <fluidsynth.h>
#include <fstream>
#include <iomanip>
#include <iterator>
// IWYU pragma: no_include <memory>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <utility>

#include "chord/Chord.hpp"
#include "chord/ChordsModel.hpp"
#include "instrument/Instrument.hpp"
#include "instrument/InstrumentEditor.hpp"
#include "interval/Interval.hpp"
#include "interval/IntervalEditor.hpp"
#include "justly/ChordColumn.hpp"
#include "other/other.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_instrument/PercussionInstrumentEditor.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "percussion_set/PercussionSetEditor.hpp"
#include "pitched_note/PitchedNote.hpp"
#include "pitched_note/PitchedNotesModel.hpp"
#include "rational/Rational.hpp"
#include "rational/RationalEditor.hpp"
#include "rows/InsertRemoveRows.hpp"
#include "rows/InsertRow.hpp"
#include "rows/Row.hpp"
#include "rows/RowsModel.hpp"
#include "rows/SetCells.hpp"
#include "song/ControlId.hpp"
#include "song/EditChildrenOrBack.hpp"
#include "song/SetStartingDouble.hpp"
#include "unpitched_note/UnpitchedNote.hpp"

// starting control bounds
static const auto MAX_GAIN = 10;
static const auto MIN_STARTING_KEY = 60;
static const auto MAX_STARTING_KEY = 440;
static const auto MIN_STARTING_TEMPO = 25;
static const auto MAX_STARTING_TEMPO = 200;

// mime types
static const auto CHORDS_CELLS_MIME = "application/prs.chords_cells+json";
static const auto PITCHED_NOTES_CELLS_MIME = "application/prs.notes_cells+json";
static const auto UNPITCHED_NOTES_CELLS_MIME =
    "application/prs.percussions_cells+json";

// play settings
static const auto GAIN_STEP = 0.1;

// get json functions
[[nodiscard]] static auto get_json_value(const nlohmann::json &json_data,
                                         const char *field) {
  Q_ASSERT(json_data.contains(field));
  return json_data[field];
}

[[nodiscard]] static auto get_json_int(const nlohmann::json &json_data,
                                       const char *field) {
  Q_ASSERT(json_data.contains(field));
  const auto &json_value = get_json_value(json_data, field);
  Q_ASSERT(json_value.is_number());
  return json_value.get<int>();
}

static void set_from_json(const nlohmann::json &json_song,
                          QDoubleSpinBox &double_editor,
                          const char *field_name) {
  if (json_song.contains(field_name)) {
    const auto &json_value = get_json_value(json_song, field_name);
    Q_ASSERT(json_value.is_number());
    double_editor.setValue(json_value.get<double>());
  }
}

[[nodiscard]] static auto
get_selection_model(const QTableView &table_view) -> QItemSelectionModel & {
  auto *selection_model_pointer = table_view.selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);
  return *selection_model_pointer;
}

[[nodiscard]] static auto
get_selection(const QTableView &table_view) -> QItemSelection {
  return get_selection_model(table_view).selection();
}

[[nodiscard]] static auto get_only_range(const QTableView &table_view) {
  const auto selection = get_selection(table_view);
  Q_ASSERT(selection.size() == 1);
  return selection.at(0);
}

[[nodiscard]] static auto get_number_of_rows(const QItemSelectionRange &range) {
  Q_ASSERT(range.isValid());
  return range.bottom() - range.top() + 1;
}

[[nodiscard]] static auto get_field_for(ModelType model_type) {
  switch (model_type) {
  case chords_type:
    return "chords";
  case pitched_notes_type:
    return "pitched_notes";
  case unpitched_notes_type:
    return "unpitched_notes";
  }
}

[[nodiscard]] static auto get_mime_description(const QString &mime_type) {
  if (mime_type == CHORDS_CELLS_MIME) {
    return SongEditor::tr("chords cells");
  }
  if (mime_type == PITCHED_NOTES_CELLS_MIME) {
    return SongEditor::tr("pitched notes cells");
  }
  if (mime_type == UNPITCHED_NOTES_CELLS_MIME) {
    return SongEditor::tr("unpitched notes cells");
  }
  return mime_type;
}

[[nodiscard]] static auto get_mime_for(ModelType model_type) -> const char * {
  switch (model_type) {
  case chords_type:
    return CHORDS_CELLS_MIME;
  case pitched_notes_type:
    return PITCHED_NOTES_CELLS_MIME;
  case unpitched_notes_type:
    return UNPITCHED_NOTES_CELLS_MIME;
  }
}

[[nodiscard]] static auto get_rows_from(const nlohmann::json &json_cells,
                                        ModelType model_type) {
  const auto *rows_field = get_field_for(model_type);
  Q_ASSERT(json_cells.contains(rows_field));
  return json_cells[rows_field];
}

template <std::derived_from<Row> SubRow>
auto add_set_cells(QUndoStack &undo_stack, RowsModel<SubRow> &rows_model,
                   int first_row_number, int left_column, int right_column,
                   const QList<SubRow> &old_rows,
                   const QList<SubRow> &new_rows) {
  undo_stack.push(
      new SetCells<SubRow>( // NOLINT(cppcoreguidelines-owning-memory)
          rows_model, first_row_number, left_column, right_column, old_rows,
          new_rows));
}

template <std::derived_from<Row> SubRow>
static void add_insert_row(QUndoStack &undo_stack,
                           RowsModel<SubRow> &rows_model, int row_number) {
  undo_stack.push(
      new InsertRow<SubRow>( // NOLINT(cppcoreguidelines-owning-memory)
          rows_model, row_number));
}

template <std::derived_from<Row> SubRow>
static void
add_insert_remove_rows(QUndoStack &undo_stack, RowsModel<SubRow> &rows_model,
                       int row_number, const QList<SubRow> &new_rows,
                       bool backwards) {
  undo_stack.push(
      new InsertRemoveRows<SubRow>( // NOLINT(cppcoreguidelines-owning-memory)
          rows_model, row_number, new_rows, backwards));
}

template <typename Item>
[[nodiscard]] static auto copy_items(const QList<Item> &items,
                                     int first_row_number, int number_of_rows) {
  QList<Item> copied;
  std::copy(items.cbegin() + first_row_number,
            items.cbegin() + first_row_number + number_of_rows,
            std::back_inserter(copied));
  return copied;
}

[[nodiscard]] static auto verify_discard_changes(QWidget& parent) {
  return QMessageBox::question(&parent, SongEditor::tr("Unsaved changes"),
                               SongEditor::tr("Discard unsaved changes?")) ==
         QMessageBox::Yes;
}

[[nodiscard]] static auto
make_file_dialog(SongEditor &song_editor, const QString &caption,
                 const QString &filter, QFileDialog::AcceptMode accept_mode,
                 const QString &suffix,
                 QFileDialog::FileMode file_mode) -> QFileDialog & {
  auto &dialog = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QFileDialog(&song_editor, caption, song_editor.current_folder,
                        filter));

  dialog.setAcceptMode(accept_mode);
  dialog.setDefaultSuffix(suffix);
  dialog.setFileMode(file_mode);

  return dialog;
}

[[nodiscard]] static auto get_selected_file(SongEditor &song_editor,
                                            QFileDialog &dialog) {
  song_editor.current_folder = dialog.directory().absolutePath();
  const auto &selected_files = dialog.selectedFiles();
  Q_ASSERT(!(selected_files.empty()));
  return selected_files[0];
}

static auto get_clipboard() -> QClipboard & {
  auto* clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  return *clipboard_pointer;
}

template <std::derived_from<Row> SubRow>
void copy_template(const SongEditor &song_editor, RowsModel<SubRow> &rows_model,
                   ModelType model_type) {
  const auto &range = get_only_range(song_editor.table_view);
  auto first_row_number = range.top();
  auto number_of_rows = get_number_of_rows(range);
  auto left_column = range.left();
  auto right_column = range.right();

  auto &rows = get_rows(rows_model);

  nlohmann::json copied_json = nlohmann::json::array();
  std::transform(
      rows.cbegin() + first_row_number,
      rows.cbegin() + first_row_number + number_of_rows,
      std::back_inserter(copied_json),
      [left_column, right_column](const SubRow &row) -> nlohmann::json {
        return row.columns_to_json(left_column, right_column);
      });

  const nlohmann::json copied(
      {{"left_column", left_column},
       {"right_column", right_column},
       {get_field_for(model_type), std::move(copied_json)}});

  std::stringstream json_text;
  json_text << std::setw(4) << copied;

  auto &new_data = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QMimeData);

  new_data.setData(get_mime_for(model_type), json_text.str().c_str());

  get_clipboard().setMimeData(&new_data);
}

static void copy(SongEditor &song_editor) {
  const auto current_model_type = song_editor.current_model_type;
  if (current_model_type == chords_type) {
    copy_template(song_editor, song_editor.chords_model, current_model_type);
  } else if (current_model_type == pitched_notes_type) {
    copy_template(song_editor, song_editor.pitched_notes_model,
                  current_model_type);
  } else {
    copy_template(song_editor, song_editor.unpitched_notes_model,
                  current_model_type);
  }
}

template <std::derived_from<Row> SubRow>
auto delete_cells_template(const SongEditor &song_editor,
                           RowsModel<SubRow> &rows_model) {
  const auto &range = get_only_range(song_editor.table_view);
  auto first_row_number = range.top();
  auto number_of_rows = get_number_of_rows(range);

  add_set_cells(
      song_editor.undo_stack, rows_model, first_row_number, range.left(),
      range.right(),
      copy_items(get_rows(rows_model), first_row_number, number_of_rows),
      QList<SubRow>(number_of_rows));
}

static void delete_cells(SongEditor &song_editor) {
  const auto current_model_type = song_editor.current_model_type;
  if (current_model_type == chords_type) {
    delete_cells_template(song_editor, song_editor.chords_model);
  } else if (current_model_type == pitched_notes_type) {
    delete_cells_template(song_editor, song_editor.pitched_notes_model);
  } else {
    delete_cells_template(song_editor, song_editor.unpitched_notes_model);
  }
}

[[nodiscard]] static auto parse_clipboard(QWidget& parent,
                                          ModelType model_type) {
  const auto &clipboard = get_clipboard();
  const auto *mime_data_pointer = clipboard.mimeData();
  Q_ASSERT(mime_data_pointer != nullptr);
  const auto &mime_data = *mime_data_pointer;

  const auto *mime_type = get_mime_for(model_type);

  if (!mime_data.hasFormat(mime_type)) {
    auto formats = mime_data.formats();
    Q_ASSERT(!(formats.empty()));
    QString message;
    QTextStream stream(&message);
    stream << SongEditor::tr("Cannot paste ")
           << get_mime_description(formats[0])
           << SongEditor::tr(" into destination needing ")
           << get_mime_description(mime_type);
    QMessageBox::warning(&parent, SongEditor::tr("MIME type error"), message);
    return nlohmann::json();
  }
  const auto &copied_text = mime_data.data(mime_type).toStdString();
  nlohmann::json copied;
  try {
    copied = nlohmann::json::parse(copied_text);
  } catch (const nlohmann::json::parse_error &parse_error) {
    QMessageBox::warning(&parent, SongEditor::tr("Parsing error"),
                         parse_error.what());
    return nlohmann::json();
  }
  if (copied.empty()) {
    QMessageBox::warning(&parent, SongEditor::tr("Empty paste"),
                         SongEditor::tr("Nothing to paste!"));
    return nlohmann::json();
  }
  try {
    switch (model_type) {
    case chords_type:
      get_chords_cells_validator().validate(copied);
      break;
    case pitched_notes_type:
      get_pitched_notes_cells_validator().validate(copied);
      break;
    case unpitched_notes_type:
      get_unpitched_notes_cells_validator().validate(copied);
      break;
    }
  } catch (const std::exception &error) {
    QMessageBox::warning(&parent, SongEditor::tr("Schema error"), error.what());
    return nlohmann::json();
  }
  return copied;
}

template <std::derived_from<Row> SubRow>
static void paste_cells_template(SongEditor &song_editor,
                                 RowsModel<SubRow> &rows_model,
                                 ModelType model_type) {
  auto first_row_number = get_only_range(song_editor.table_view).top();

  const auto json_cells = parse_clipboard(song_editor, model_type);
  if (json_cells.empty()) {
    return;
  }
  const auto &json_rows = get_rows_from(json_cells, model_type);

  auto &rows = get_rows(rows_model);

  auto number_of_rows =
      std::min({static_cast<int>(json_rows.size()),
                static_cast<int>(rows.size()) - first_row_number});

  QList<SubRow> new_rows;
  partial_json_to_rows(new_rows, json_rows, number_of_rows);
  add_set_cells(song_editor.undo_stack, rows_model, first_row_number,
                get_json_int(json_cells, "left_column"),
                get_json_int(json_cells, "right_column"),
                copy_items(rows, first_row_number, number_of_rows),
                std::move(new_rows));
}

template <std::derived_from<Row> SubRow>
static void paste_insert_template(SongEditor &song_editor,
                                  RowsModel<SubRow> &rows_model,
                                  ModelType model_type, int row_number) {
  const auto json_cells = parse_clipboard(song_editor, model_type);
  if (json_cells.empty()) {
    return;
  }
  const auto &json_rows = get_rows_from(json_cells, model_type);

  QList<SubRow> new_rows;
  json_to_rows(new_rows, json_rows);
  add_insert_remove_rows(song_editor.undo_stack, rows_model, row_number,
                         new_rows, false);
}

static void paste_insert(SongEditor &song_editor, int row_number) {
  const auto current_model_type = song_editor.current_model_type;
  if (current_model_type == chords_type) {
    paste_insert_template(song_editor, song_editor.chords_model,
                          current_model_type, row_number);
  } else if (current_model_type == pitched_notes_type) {
    paste_insert_template(song_editor, song_editor.pitched_notes_model,
                          current_model_type, row_number);
  } else {
    paste_insert_template(song_editor, song_editor.unpitched_notes_model,
                          current_model_type, row_number);
  }
}

static void insert_model_row(SongEditor &song_editor, int row_number) {
  const auto current_model_type = song_editor.current_model_type;
  auto &undo_stack = song_editor.undo_stack;
  if (current_model_type == chords_type) {
    add_insert_row(undo_stack, song_editor.chords_model, row_number);
  } else if (current_model_type == pitched_notes_type) {
    add_insert_row(undo_stack, song_editor.pitched_notes_model, row_number);
  } else {
    add_insert_row(undo_stack, song_editor.unpitched_notes_model, row_number);
  }
}

template <std::derived_from<Row> SubRow>
static void remove_rows_template(SongEditor &song_editor,
                                 RowsModel<SubRow> &rows_model) {
  const auto &range = get_only_range(song_editor.table_view);
  auto first_row_number = range.top();
  auto number_of_rows = get_number_of_rows(range);

  auto &rows = get_rows(rows_model);
  add_insert_remove_rows(song_editor.undo_stack, rows_model, first_row_number,
                         copy_items(rows, first_row_number, number_of_rows),
                         true);
}

static void add_edit_children_or_back(SongEditor &song_editor, int chord_number,
                                      bool is_pitched, bool backwards) {
  song_editor.undo_stack.push(
      new EditChildrenOrBack( // NOLINT(cppcoreguidelines-owning-memory)
          song_editor, chord_number, is_pitched, backwards));
}

static void set_double(SongEditor &song_editor, ControlId command_id,
                       double new_value) {
  song_editor.undo_stack.push(
      new SetStartingDouble( // NOLINT(cppcoreguidelines-owning-memory)
          song_editor, command_id, new_value));
}

static void update_actions(const SongEditor &song_editor) {
  auto anything_selected = !get_selection(song_editor.table_view).empty();

  song_editor.cut_action.setEnabled(anything_selected);
  song_editor.copy_action.setEnabled(anything_selected);
  song_editor.paste_over_action.setEnabled(anything_selected);
  song_editor.paste_after_action.setEnabled(anything_selected);
  song_editor.insert_after_action.setEnabled(anything_selected);
  song_editor.delete_action.setEnabled(anything_selected);
  song_editor.remove_rows_action.setEnabled(anything_selected);
  song_editor.play_action.setEnabled(anything_selected);
}

void set_model(SongEditor &song_editor, QAbstractItemModel &model) {
  song_editor.table_view.setModel(&model);
  update_actions(song_editor);

  SongEditor::connect(&get_selection_model(song_editor.table_view),
                      &QItemSelectionModel::selectionChanged, &song_editor,
                      [&song_editor]() { update_actions(song_editor); });
}

static void connect_model(const SongEditor &song_editor,
                          const QAbstractItemModel &model) {
  SongEditor::connect(&model, &QAbstractItemModel::rowsInserted, &song_editor,
                      [&song_editor]() { update_actions(song_editor); });
  SongEditor::connect(&model, &QAbstractItemModel::rowsRemoved, &song_editor,
                      [&song_editor]() { update_actions(song_editor); });
}

SongEditor::SongEditor()
    : player(Player(*this)),
      current_folder(
          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)),
      undo_stack(*(new QUndoStack(this))),
      gain_editor(*(new QDoubleSpinBox(this))),
      starting_key_editor(*(new QDoubleSpinBox(this))),
      starting_velocity_editor(*(new QDoubleSpinBox(this))),
      starting_tempo_editor(*(new QDoubleSpinBox(this))),
      editing_chord_text(*(new QLabel(SongEditor::tr("Editing chords")))),
      table_view(*(new QTableView(this))),
      chords_model(ChordsModel(undo_stack, song)),
      pitched_notes_model(PitchedNotesModel(undo_stack, song)),
      unpitched_notes_model(RowsModel<UnpitchedNote>(undo_stack)),
      back_to_chords_action(
          *(new QAction(SongEditor::tr("&Back to chords"), this))),
      insert_after_action(*(new QAction(SongEditor::tr("&After"), this))),
      insert_into_action(*(new QAction(SongEditor::tr("&Into start"), this))),
      delete_action(*(new QAction(SongEditor::tr("&Delete"), this))),
      remove_rows_action(*(new QAction(SongEditor::tr("&Remove rows"), this))),
      cut_action(*(new QAction(SongEditor::tr("&Cut"), this))),
      copy_action(*(new QAction(SongEditor::tr("&Copy"), this))),
      paste_over_action(*(new QAction(SongEditor::tr("&Over"), this))),
      paste_into_action(*(new QAction(SongEditor::tr("&Into start"), this))),
      paste_after_action(*(new QAction(SongEditor::tr("&After"), this))),
      play_action(*(new QAction(SongEditor::tr("&Play selection"), this))),
      stop_playing_action(
          *(new QAction(SongEditor::tr("&Stop playing"), this))),
      save_action(*(new QAction(SongEditor::tr("&Save"), this))),
      open_action(*(new QAction(SongEditor::tr("&Open"), this))) {
  statusBar()->showMessage(SongEditor::tr(""));

  auto &factory = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QItemEditorFactory);
  factory.registerEditor(
      qMetaTypeId<Rational>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          RationalEditor>);
  factory.registerEditor(
      qMetaTypeId<const PercussionInstrument *>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          PercussionInstrumentEditor>);
  factory.registerEditor(
      qMetaTypeId<const PercussionSet *>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          PercussionSetEditor>);
  factory.registerEditor(
      qMetaTypeId<const Instrument *>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          InstrumentEditor>);
  factory.registerEditor(
      qMetaTypeId<Interval>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          IntervalEditor>);
  factory.registerEditor(
      qMetaTypeId<QString>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          QLineEdit>);
  factory.registerEditor(
      qMetaTypeId<int>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          QSpinBox>);
  QItemEditorFactory::setDefaultFactory(&factory);

  auto &controls = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QFrame(this));
  controls.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  auto &dock_widget = *(new QDockWidget(SongEditor::tr("Controls"), this));

  auto *menu_bar_pointer = menuBar();
  Q_ASSERT(menu_bar_pointer != nullptr);
  auto &menu_bar = *(menu_bar_pointer);

  auto &file_menu = *(new QMenu(SongEditor::tr("&File"), this));

  file_menu.addAction(&open_action);
  connect(&open_action, &QAction::triggered, this, [this]() {
    if (undo_stack.isClean() || verify_discard_changes(*this)) {
      auto &dialog = make_file_dialog(
          *this, SongEditor::tr("Open — Justly"), "JSON file (*.json)",
          QFileDialog::AcceptOpen, ".json", QFileDialog::ExistingFile);
      if (dialog.exec() != 0) {
        open_file(*this, get_selected_file(*this, dialog));
      }
    }
  });
  open_action.setShortcuts(QKeySequence::Open);

  file_menu.addSeparator();

  save_action.setShortcuts(QKeySequence::Save);
  connect(&save_action, &QAction::triggered, this,
          [this]() { save_as_file(*this, current_file); });
  file_menu.addAction(&save_action);
  save_action.setEnabled(false);

  auto &save_as_action =
      *(new QAction(SongEditor::tr("&Save As..."), &file_menu));
  save_as_action.setShortcuts(QKeySequence::SaveAs);
  connect(&save_as_action, &QAction::triggered, this, [this]() {
    auto &dialog = make_file_dialog(
        *this, SongEditor::tr("Save As — Justly"), "JSON file (*.json)",
        QFileDialog::AcceptSave, ".json", QFileDialog::AnyFile);

    if (dialog.exec() != 0) {
      save_as_file(*this, get_selected_file(*this, dialog));
    }
  });
  file_menu.addAction(&save_as_action);
  save_as_action.setEnabled(true);

  auto &export_action =
      *(new QAction(SongEditor::tr("&Export recording"), &file_menu));
  connect(&export_action, &QAction::triggered, this, [this]() {
    auto &dialog = make_file_dialog(*this, SongEditor::tr("Export — Justly"),
                                    "WAV file (*.wav)", QFileDialog::AcceptSave,
                                    ".wav", QFileDialog::AnyFile);
    dialog.setLabelText(QFileDialog::Accept, "Export");
    if (dialog.exec() != 0) {
      export_song_to_file(player, song, get_selected_file(*this, dialog));
    }
  });
  file_menu.addAction(&export_action);

  menu_bar.addMenu(&file_menu);

  auto &edit_menu = *(new QMenu(SongEditor::tr("&Edit"), this));

  auto *undo_action_pointer = undo_stack.createUndoAction(&edit_menu);
  Q_ASSERT(undo_action_pointer != nullptr);
  auto &undo_action = *undo_action_pointer;
  undo_action.setShortcuts(QKeySequence::Undo);
  edit_menu.addAction(&undo_action);

  auto *redo_action_pointer = undo_stack.createRedoAction(&edit_menu);
  Q_ASSERT(redo_action_pointer != nullptr);
  auto &redo_action = *redo_action_pointer;
  redo_action.setShortcuts(QKeySequence::Redo);
  edit_menu.addAction(&redo_action);

  edit_menu.addSeparator();

  cut_action.setEnabled(false);
  cut_action.setShortcuts(QKeySequence::Cut);
  connect(&cut_action, &QAction::triggered, this, [this]() {
    copy(*this);
    delete_cells(*this);
  });
  edit_menu.addAction(&cut_action);

  copy_action.setEnabled(false);
  copy_action.setShortcuts(QKeySequence::Copy);
  connect(&copy_action, &QAction::triggered, this, [this]() { copy((*this)); });
  edit_menu.addAction(&copy_action);

  auto &paste_menu = *(new QMenu(SongEditor::tr("&Paste"), &edit_menu));
  edit_menu.addMenu(&paste_menu);

  paste_over_action.setEnabled(false);
  connect(&paste_over_action, &QAction::triggered, this, [this]() {
    if (current_model_type == chords_type) {
      paste_cells_template(*this, chords_model, current_model_type);
    } else if (current_model_type == pitched_notes_type) {
      paste_cells_template(*this, pitched_notes_model, current_model_type);
    } else {
      paste_cells_template(*this, unpitched_notes_model, current_model_type);
    }
  });
  paste_over_action.setShortcuts(QKeySequence::Paste);
  paste_menu.addAction(&paste_over_action);

  paste_into_action.setEnabled(true);
  connect(&paste_into_action, &QAction::triggered, this,
          [this]() { paste_insert(*this, 0); });
  paste_menu.addAction(&paste_into_action);

  paste_after_action.setEnabled(false);
  connect(&paste_after_action, &QAction::triggered, this, [this]() {
    paste_insert(*this, get_only_range(table_view).bottom() + 1);
  });
  paste_menu.addAction(&paste_after_action);

  edit_menu.addSeparator();

  auto &insert_menu = *(new QMenu(SongEditor::tr("&Insert"), &edit_menu));

  edit_menu.addMenu(&insert_menu);

  insert_after_action.setEnabled(false);
  insert_after_action.setShortcuts(QKeySequence::InsertLineSeparator);
  connect(&insert_after_action, &QAction::triggered, this, [this]() {
    insert_model_row(*this, get_only_range(table_view).bottom() + 1);
  });
  insert_menu.addAction(&insert_after_action);

  insert_into_action.setEnabled(true);
  insert_into_action.setShortcuts(QKeySequence::AddTab);
  connect(&insert_into_action, &QAction::triggered, this,
          [this]() { insert_model_row(*this, 0); });
  insert_menu.addAction(&insert_into_action);

  delete_action.setEnabled(false);
  delete_action.setShortcuts(QKeySequence::Delete);
  connect(&delete_action, &QAction::triggered, this,
          [this]() { delete_cells(*this); });
  edit_menu.addAction(&delete_action);

  remove_rows_action.setEnabled(false);
  remove_rows_action.setShortcuts(QKeySequence::DeleteStartOfWord);
  connect(&remove_rows_action, &QAction::triggered, this, [this]() {
    if (current_model_type == chords_type) {
      remove_rows_template(*this, chords_model);
    } else if (current_model_type == pitched_notes_type) {
      remove_rows_template(*this, pitched_notes_model);
    } else {
      remove_rows_template(*this, unpitched_notes_model);
    }
  });
  edit_menu.addAction(&remove_rows_action);

  edit_menu.addSeparator();

  back_to_chords_action.setEnabled(false);
  back_to_chords_action.setShortcuts(QKeySequence::Back);
  connect(&back_to_chords_action, &QAction::triggered, this, [this]() {
    add_edit_children_or_back(*this, current_chord_number,
                              current_model_type == pitched_notes_type, true);
  });
  edit_menu.addAction(&back_to_chords_action);

  menu_bar.addMenu(&edit_menu);

  auto &view_menu = *(new QMenu(SongEditor::tr("&View"), this));

  auto &view_controls_checkbox =
      *(new QAction(SongEditor::tr("&Controls"), &view_menu));

  view_controls_checkbox.setCheckable(true);
  view_controls_checkbox.setChecked(true);
  connect(&view_controls_checkbox, &QAction::toggled, &dock_widget,
          &QWidget::setVisible);
  view_menu.addAction(&view_controls_checkbox);

  menu_bar.addMenu(&view_menu);

  auto &play_menu = *(new QMenu(SongEditor::tr("&Play"), this));

  play_action.setEnabled(false);
  play_action.setShortcuts(QKeySequence::Print);
  connect(&play_action, &QAction::triggered, this, [this]() {
    const auto &range = get_only_range(table_view);
    auto first_row_number = range.top();
    auto number_of_rows = get_number_of_rows(range);

    stop_playing(player);
    initialize_play(player, song);
    if (current_model_type == chords_type) {
      modulate_before_chord(player, song, first_row_number);
      play_chords(player, song, first_row_number, number_of_rows);
    } else {
      modulate_before_chord(player, song, current_chord_number);
      const auto &chord = song.chords.at(current_chord_number);
      modulate(player, chord);
      if (current_model_type == pitched_notes_type) {
        play_pitched_notes(player, current_chord_number, chord,
                           first_row_number, number_of_rows);
      } else {
        play_unpitched_notes(player, current_chord_number, chord,
                             first_row_number, number_of_rows);
      }
    }
  });
  play_menu.addAction(&play_action);

  stop_playing_action.setEnabled(true);
  play_menu.addAction(&stop_playing_action);
  connect(&stop_playing_action, &QAction::triggered, this,
          [this]() { stop_playing(player); });
  stop_playing_action.setShortcuts(QKeySequence::Cancel);

  menu_bar.addMenu(&play_menu);

  auto &controls_form = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QFormLayout(&controls));

  gain_editor.setMinimum(0);
  gain_editor.setMaximum(MAX_GAIN);
  gain_editor.setSingleStep(GAIN_STEP);
  connect(&gain_editor, &QDoubleSpinBox::valueChanged, this,
          [this](double new_value) { set_double(*this, gain_id, new_value); });
  set_double_directly(*this, gain_id, song.gain);
  controls_form.addRow(SongEditor::tr("&Gain:"), &gain_editor);

  starting_key_editor.setMinimum(MIN_STARTING_KEY);
  starting_key_editor.setMaximum(MAX_STARTING_KEY);
  starting_key_editor.setDecimals(1);
  starting_key_editor.setSuffix(" hz");

  starting_key_editor.setValue(this->song.starting_key);

  connect(&starting_key_editor, &QDoubleSpinBox::valueChanged, this,
          [this](double new_value) {
            set_double(*this, starting_key_id, new_value);
          });
  controls_form.addRow(SongEditor::tr("Starting &key:"), &starting_key_editor);

  starting_velocity_editor.setMinimum(0);
  starting_velocity_editor.setMaximum(MAX_VELOCITY);
  starting_velocity_editor.setDecimals(1);
  starting_velocity_editor.setValue(this->song.starting_velocity);
  connect(&starting_velocity_editor, &QDoubleSpinBox::valueChanged, this,
          [this](double new_value) {
            set_double(*this, starting_velocity_id, new_value);
          });
  controls_form.addRow(SongEditor::tr("Starting &velocity:"),
                       &starting_velocity_editor);

  starting_tempo_editor.setMinimum(MIN_STARTING_TEMPO);
  starting_tempo_editor.setValue(this->song.gain);
  starting_tempo_editor.setDecimals(1);
  starting_tempo_editor.setSuffix(" bpm");
  starting_tempo_editor.setMaximum(MAX_STARTING_TEMPO);

  set_model(*this, chords_model);
  connect(&starting_tempo_editor, &QDoubleSpinBox::valueChanged, this,
          [this](double new_value) {
            set_double(*this, starting_tempo_id, new_value);
          });
  controls_form.addRow(SongEditor::tr("Starting &tempo:"),
                       &starting_tempo_editor);

  connect_model(*this, chords_model);
  connect_model(*this, pitched_notes_model);
  connect_model(*this, unpitched_notes_model);

  dock_widget.setWidget(&controls);
  dock_widget.setFeatures(QDockWidget::NoDockWidgetFeatures);
  addDockWidget(Qt::LeftDockWidgetArea, &dock_widget);

  table_view.setSelectionMode(QAbstractItemView::ContiguousSelection);
  table_view.setSelectionBehavior(QAbstractItemView::SelectItems);
  table_view.setSizeAdjustPolicy(
      QAbstractScrollArea::AdjustToContentsOnFirstShow);

  auto *header_pointer = table_view.horizontalHeader();
  Q_ASSERT(header_pointer != nullptr);
  header_pointer->setSectionResizeMode(QHeaderView::ResizeToContents);

  table_view.setMouseTracking(true);

  connect(&table_view, &QAbstractItemView::doubleClicked, this,
          [this](const QModelIndex &index) {
            if (current_model_type == chords_type) {
              auto row = index.row();
              auto column = index.column();
              auto is_notes_column = column == chord_pitched_notes_column;
              if (is_notes_column || (column == chord_unpitched_notes_column)) {
                add_edit_children_or_back(*this, row, is_notes_column, false);
              }
            }
          });

  setWindowTitle("Justly");

  auto &table_column = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QWidget(this));
  auto &table_column_layout = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QVBoxLayout(&table_column));
  table_column_layout.addWidget(&editing_chord_text);
  table_column_layout.addWidget(&table_view);
  setCentralWidget(&table_column);

  connect(&undo_stack, &QUndoStack::cleanChanged, this, [this]() {
    save_action.setEnabled(!undo_stack.isClean() && !current_file.isEmpty());
  });

  const auto *primary_screen_pointer = QGuiApplication::primaryScreen();
  Q_ASSERT(primary_screen_pointer != nullptr);
  const auto full_size = primary_screen_pointer->availableGeometry();
  resize(full_size.width(), full_size.height());

  undo_stack.clear();
  undo_stack.setClean();
}

SongEditor::~SongEditor() { undo_stack.disconnect(); }

void SongEditor::closeEvent(QCloseEvent *close_event_pointer) {
  if (!undo_stack.isClean() && !verify_discard_changes(*this)) {
    close_event_pointer->ignore();
    return;
  }
  QMainWindow::closeEvent(close_event_pointer);
}

void is_chords_now(const SongEditor &song_editor, bool is_chords) {
  song_editor.back_to_chords_action.setEnabled(!is_chords);
  song_editor.open_action.setEnabled(is_chords);
}

void set_double_directly(SongEditor &song_editor, ControlId command_id,
                         double new_value) {
  QDoubleSpinBox *spinbox_pointer = nullptr;
  auto &song = song_editor.song;
  switch (command_id) {
  case gain_id:
    spinbox_pointer = &song_editor.gain_editor;
    song.gain = new_value;
    fluid_synth_set_gain(song_editor.player.synth_pointer,
                         static_cast<float>(new_value));
    break;
  case starting_key_id:
    spinbox_pointer = &song_editor.starting_key_editor;
    song.starting_key = new_value;
    break;
  case starting_velocity_id:
    spinbox_pointer = &song_editor.starting_velocity_editor;
    song.starting_velocity = new_value;
    break;
  case starting_tempo_id:
    spinbox_pointer = &song_editor.starting_tempo_editor;
    song.starting_tempo = new_value;
  }
  Q_ASSERT(spinbox_pointer != nullptr);
  auto &spin_box = *spinbox_pointer;
  spin_box.blockSignals(true);
  spin_box.setValue(new_value);
  spin_box.blockSignals(false);
};

void open_file(SongEditor &song_editor, const QString &filename) {
  auto &chords = song_editor.song.chords;
  auto &chords_model = song_editor.chords_model;
  auto &undo_stack = song_editor.undo_stack;

  std::ifstream file_io(filename.toStdString().c_str());
  nlohmann::json json_song;
  try {
    json_song = nlohmann::json::parse(file_io);
  } catch (const nlohmann::json::parse_error &parse_error) {
    QMessageBox::warning(&song_editor, SongEditor::tr("Parsing error"),
                         parse_error.what());
    return;
  }
  file_io.close();

  static auto song_validator = make_validator(
      "Song",
      nlohmann::json(
          {{"description", "A Justly song in JSON format"},
           {"type", "object"},
           {"required", nlohmann::json({
                            "starting_key",
                            "starting_tempo",
                            "starting_velocity",
                        })},
           {"properties",
            nlohmann::json(
                {{"gain",
                  nlohmann::json({{"type", "number"},
                                  {"description", "the gain (speaker volume)"},
                                  {"minimum", 0},
                                  {"maximum", MAX_GAIN}})},
                 {"starting_key",
                  nlohmann::json({{"type", "number"},
                                  {"description", "the starting key, in Hz"},
                                  {"minimum", MIN_STARTING_KEY},
                                  {"maximum", MAX_STARTING_KEY}})},
                 {"starting_tempo",
                  nlohmann::json({{"type", "number"},
                                  {"description", "the starting tempo, in bpm"},
                                  {"minimum", MIN_STARTING_TEMPO},
                                  {"maximum", MAX_STARTING_TEMPO}})},
                 {"starting_velocity",
                  nlohmann::json(
                      {{"type", "number"},
                       {"description",
                        "the starting velocity (pitched_note force)"},
                       {"minimum", 0},
                       {"maximum", MAX_VELOCITY}})},
                 {"chords", get_chords_schema()}})}}));
  try {
    song_validator.validate(json_song);
  } catch (const std::exception &error) {
    QMessageBox::warning(&song_editor, SongEditor::tr("Schema error"),
                         error.what());
    return;
  }

  set_from_json(json_song, song_editor.gain_editor, "gain");
  set_from_json(json_song, song_editor.starting_key_editor, "starting_key");
  set_from_json(json_song, song_editor.starting_velocity_editor,
                "starting_velocity");
  set_from_json(json_song, song_editor.starting_tempo_editor, "starting_tempo");

  if (!chords.empty()) {
    remove_rows(chords_model, 0, static_cast<int>(chords.size()));
  }

  if (json_song.contains("chords")) {
    const auto &json_chords = json_song["chords"];

    chords_model.begin_insert_rows(static_cast<int>(chords.size()),
                                   static_cast<int>(json_chords.size()));
    json_to_rows(chords, json_chords);
    chords_model.end_insert_rows();
  }

  song_editor.current_file = filename;

  undo_stack.clear();
  undo_stack.setClean();
}

void save_as_file(SongEditor &song_editor, const QString &filename) {
  const auto &song = song_editor.song;

  std::ofstream file_io(filename.toStdString().c_str());

  nlohmann::json json_song;
  json_song["gain"] = song.gain;
  json_song["starting_key"] = song.starting_key;
  json_song["starting_tempo"] = song.starting_tempo;
  json_song["starting_velocity"] = song.starting_velocity;

  add_rows_to_json(json_song, song.chords, "chords");

  file_io << std::setw(4) << json_song;
  file_io.close();
  song_editor.current_file = filename;

  song_editor.undo_stack.setClean();
}

void export_to_file(SongEditor &song_editor, const QString &output_file) {
  export_song_to_file(song_editor.player, song_editor.song, output_file);
}