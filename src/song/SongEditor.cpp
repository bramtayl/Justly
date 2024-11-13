#include "song/SongEditor.hpp"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QAction>
#include <QBoxLayout>
#include <QClipboard>
#include <QCloseEvent>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGuiApplication>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
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
#include <concepts>
#include <exception>
#include <fluidsynth.h>
#include <fstream>
#include <iomanip>
#include <iterator>
// IWYU pragma: no_include <memory>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <utility>

#include "abstract_rational/rational/Rational.hpp"
#include "justly/ChordColumn.hpp"
#include "other/other.hpp"
#include "row/InsertRemoveRows.hpp"
#include "row/InsertRow.hpp"
#include "row/Row.hpp"
#include "row/RowsModel.hpp"
#include "row/SetCells.hpp"
#include "row/chord/Chord.hpp"
#include "row/chord/ChordsModel.hpp"
#include "row/note/pitched_note/PitchedNote.hpp"
#include "row/note/pitched_note/PitchedNotesModel.hpp"
#include "row/note/unpitched_note/UnpitchedNote.hpp"
#include "song/ControlId.hpp"
#include "song/EditChildrenOrBack.hpp"
#include "song/SetStartingDouble.hpp"

// starting control bounds
static const auto DEFAULT_GAIN = 5;
static const auto MAX_GAIN = 10;
static const auto MIN_STARTING_KEY = 60;
static const auto MAX_STARTING_KEY = 440;
static const auto MIN_STARTING_TEMPO = 25;
static const auto MAX_STARTING_TEMPO = 200;

// play settings
static const auto GAIN_STEP = 0.1;

// get json functions
[[nodiscard]] static auto get_json_value(const nlohmann::json &json_data,
                                         const char *field) {
  Q_ASSERT(json_data.is_object());
  Q_ASSERT(json_data.contains(field));
  return json_data[field];
}

[[nodiscard]] static auto get_json_int(const nlohmann::json &json_data,
                                       const char *field) {
  const auto &json_value = get_json_value(json_data, field);
  Q_ASSERT(json_value.is_number());
  return json_value.get<int>();
}

static void set_double_from_json(const nlohmann::json &json_song,
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
  return get_reference(table_view.selectionModel());
}

[[nodiscard]] static auto
get_selection(const QTableView &table_view) -> QItemSelection {
  return get_selection_model(table_view).selection();
}

template <typename Iterable>
[[nodiscard]] static auto get_only(const Iterable &iterable) {
  Q_ASSERT(iterable.size() == 1);
  return iterable.at(0);
}

[[nodiscard]] static auto get_only_range(const QTableView &table_view) {
  return get_only(get_selection(table_view));
}

[[nodiscard]] static auto get_number_of_rows(const QItemSelectionRange &range) {
  Q_ASSERT(range.isValid());
  return range.bottom() - range.top() + 1;
}

template <std::derived_from<Row> SubRow>
static auto get_cells_mime() -> QString & {
  static auto cells_mime = [] {
    QString cells_mime;
    QTextStream stream(&cells_mime);
    stream << "application/prs." << SubRow::get_plural_field_for()
           << "_cells+json";
    return cells_mime;
  }();
  return cells_mime;
}

[[nodiscard]] static auto get_mime_description(const QString &mime_type) {
  if (mime_type == get_cells_mime<Chord>()) {
    return SongEditor::tr("chords cells");
  }
  if (mime_type == get_cells_mime<PitchedNote>()) {
    return SongEditor::tr("pitched notes cells");
  }
  if (mime_type == get_cells_mime<UnpitchedNote>()) {
    return SongEditor::tr("unpitched notes cells");
  }
  return mime_type;
}

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto
add_set_cells(QUndoStack &undo_stack, RowsModel<SubRow> &rows_model,
              int first_row_number, int left_column, int right_column,
              const QList<SubRow> &old_rows, const QList<SubRow> &new_rows) {
  undo_stack.push(
      new SetCells<SubRow>( // NOLINT(cppcoreguidelines-owning-memory)
          rows_model, first_row_number, left_column, right_column, old_rows,
          new_rows));
}

template <std::derived_from<Row> SubRow>
static void add_insert_row(QUndoStack &undo_stack,
                           RowsModel<SubRow> &rows_model, int row_number,
                           const SubRow &new_row) {
  undo_stack.push(
      new InsertRow<SubRow>( // NOLINT(cppcoreguidelines-owning-memory)
          rows_model, row_number, new_row));
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

[[nodiscard]] static auto verify_discard_changes(QWidget &parent) {
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
  return get_only(dialog.selectedFiles());
}

[[nodiscard]] static auto get_clipboard() -> QClipboard & {
  return get_reference(QGuiApplication::clipboard());
}

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto
get_rows(RowsModel<SubRow> &rows_model) -> QList<SubRow> & {
  return get_reference(rows_model.rows_pointer);
};

template <std::derived_from<Row> SubRow>
static void copy_template(const SongEditor &song_editor,
                          RowsModel<SubRow> &rows_model) {
  const auto &range = get_only_range(song_editor.table_view);
  auto first_row_number = range.top();
  auto left_column = range.left();
  auto right_column = range.right();

  auto &rows = get_rows(rows_model);

  nlohmann::json copied_json = nlohmann::json::array();
  std::transform(
      rows.cbegin() + first_row_number,
      rows.cbegin() + first_row_number + get_number_of_rows(range),
      std::back_inserter(copied_json),
      [left_column, right_column](const SubRow &row) -> nlohmann::json {
        return row.columns_to_json(left_column, right_column);
      });

  const nlohmann::json copied(
      {{"left_column", left_column},
       {"right_column", right_column},
       {SubRow::get_plural_field_for(), std::move(copied_json)}});

  std::stringstream json_text;
  json_text << std::setw(4) << copied;

  auto &new_data = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QMimeData);

  new_data.setData(get_cells_mime<SubRow>(), json_text.str().c_str());

  get_clipboard().setMimeData(&new_data);
}

static void copy_selection(SongEditor &song_editor) {
  switch (song_editor.current_model_type) {
  case chords_type:
    copy_template(song_editor, song_editor.chords_model);
    return;
  case pitched_notes_type:
    copy_template(song_editor, song_editor.pitched_notes_model);
    return;
  case unpitched_notes_type:
    copy_template(song_editor, song_editor.unpitched_notes_model);
    return;
  default:
    Q_ASSERT(false);
    return;
  }
}

template <std::derived_from<Row> SubRow>
static auto delete_cells_template(const SongEditor &song_editor,
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

static auto make_validator(const char *title, nlohmann::json json) {
  json["$schema"] = "http://json-schema.org/draft-07/schema#";
  json["title"] = title;
  return nlohmann::json_schema::json_validator(json);
}

static void delete_cells(SongEditor &song_editor) {
  switch (song_editor.current_model_type) {
  case chords_type:
    delete_cells_template(song_editor, song_editor.chords_model);
    return;
  case pitched_notes_type:
    delete_cells_template(song_editor, song_editor.pitched_notes_model);
    return;
  case unpitched_notes_type:
    delete_cells_template(song_editor, song_editor.unpitched_notes_model);
    return;
  default:
    Q_ASSERT(false);
    break;
  }
}

static auto get_required_object_schema(
    const char *description, const nlohmann::json &required_json,
    const nlohmann::json &properties_json) -> nlohmann::json {
  return nlohmann::json({{"type", "object"},
                         {"description", description},
                         {"required", required_json},
                         {"properties", properties_json}});
};

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto parse_clipboard(QWidget &parent) {
  const auto &mime_data = get_const_reference(get_clipboard().mimeData());
  const auto &mime_type = get_cells_mime<SubRow>();
  if (!mime_data.hasFormat(mime_type)) {
    auto formats = mime_data.formats();
    if (formats.empty()) {
      QMessageBox::warning(&parent, SongEditor::tr("Empty paste error"), SongEditor::tr("Nothing to paste!"));
      return nlohmann::json();
    };
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
  static const auto cells_validator = []() {
    auto last_column = SubRow::get_number_of_columns() - 1;
    nlohmann::json cells_schema(
        {{"left_column",
          get_number_schema("integer", "left_column", 0, last_column)},
         {"right_column",
          get_number_schema("integer", "right_column", 0, last_column)}});
    add_row_array_schema<SubRow>(cells_schema);
    return make_validator("cells",
                          get_required_object_schema(
                              "cells",
                              nlohmann::json({"left_column", "right_column",
                                              SubRow::get_plural_field_for()}),
                              cells_schema));
  }();

  try {
    cells_validator.validate(copied);
  } catch (const std::exception &error) {
    QMessageBox::warning(&parent, SongEditor::tr("Schema error"), error.what());
    return nlohmann::json();
  }
  return copied;
}

template <std::derived_from<Row> SubRow>
static void paste_cells_template(SongEditor &song_editor,
                                 RowsModel<SubRow> &rows_model) {
  auto first_row_number = get_only_range(song_editor.table_view).top();

  const auto json_cells = parse_clipboard<SubRow>(song_editor);
  if (json_cells.empty()) {
    return;
  }
  const auto &json_rows =
      get_json_value(json_cells, SubRow::get_plural_field_for());

  const auto &rows = get_rows(rows_model);

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
                                  int row_number) {
  const auto json_cells = parse_clipboard<SubRow>(song_editor);
  if (json_cells.empty()) {
    return;
  }
  const auto &json_rows =
      get_json_value(json_cells, SubRow::get_plural_field_for());

  QList<SubRow> new_rows;
  json_to_rows(new_rows, json_rows);
  add_insert_remove_rows(song_editor.undo_stack, rows_model, row_number,
                         new_rows, false);
}

static void paste_insert(SongEditor &song_editor, int row_number) {
  switch (song_editor.current_model_type) {
  case chords_type:
    paste_insert_template(song_editor, song_editor.chords_model, row_number);
    return;
  case pitched_notes_type:
    paste_insert_template(song_editor, song_editor.pitched_notes_model,
                          row_number);
    return;
  case unpitched_notes_type:
    paste_insert_template(song_editor, song_editor.unpitched_notes_model,
                          row_number);
    return;
  default:
    Q_ASSERT(false);
    return;
  }
}

static void insert_model_row(SongEditor &song_editor, int row_number) {
  const auto current_model_type = song_editor.current_model_type;
  auto &undo_stack = song_editor.undo_stack;
  if (current_model_type == chords_type) {
    add_insert_row(undo_stack, song_editor.chords_model, row_number, Chord());
  } else {
    const auto &chord_beats =
        song_editor.song.chords[song_editor.current_chord_number].beats;
    if (current_model_type == pitched_notes_type) {
      PitchedNote new_pitched_note;
      new_pitched_note.beats = chord_beats;
      add_insert_row(undo_stack, song_editor.pitched_notes_model, row_number,
                     new_pitched_note);
    } else {
      UnpitchedNote new_unpitched_note;
      new_unpitched_note.beats = chord_beats;
      add_insert_row(undo_stack, song_editor.unpitched_notes_model, row_number,
                     new_unpitched_note);
    }
  }
}

template <std::derived_from<Row> SubRow>
static void remove_rows_template(SongEditor &song_editor,
                                 RowsModel<SubRow> &rows_model) {
  const auto &range = get_only_range(song_editor.table_view);
  auto first_row_number = range.top();
  auto number_of_rows = get_number_of_rows(range);

  add_insert_remove_rows(
      song_editor.undo_stack, rows_model, first_row_number,
      copy_items(get_rows(rows_model), first_row_number, number_of_rows), true);
}

static void add_edit_children_or_back(SongEditor &song_editor, int chord_number,
                                      bool is_pitched, bool backwards) {
  song_editor.undo_stack.push(
      new EditChildrenOrBack( // NOLINT(cppcoreguidelines-owning-memory)
          song_editor, chord_number, is_pitched, backwards));
}

static void set_double(SongEditor &song_editor, QDoubleSpinBox &spin_box,
                       ControlId command_id, double old_value,
                       double new_value) {
  song_editor.undo_stack.push(
      new SetStartingDouble( // NOLINT(cppcoreguidelines-owning-memory)
          song_editor, spin_box, command_id, old_value, new_value));
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

template <typename Functor>
static void add_menu_action(SongEditor &song_editor, QMenu &menu,
                            QAction &action, Functor &&trigger_action,
                            QKeySequence::StandardKey key_sequence,
                            bool enabled = true) {
  action.setShortcuts(key_sequence);
  action.setEnabled(enabled);
  menu.addAction(&action);
  QObject::connect(&action, &QAction::triggered, &song_editor, trigger_action);
}

template <typename Functor>
static void add_control(SongEditor &song_editor, QFormLayout &controls_form,
                        const char *label, QDoubleSpinBox &spin_box,
                        double starting_value, int minimum, int maximum,
                        double single_step, int decimals, const char *suffix,
                        Functor &&value_action) {
  spin_box.setMinimum(minimum);
  spin_box.setMaximum(maximum);
  spin_box.setSingleStep(single_step);
  spin_box.setDecimals(decimals);
  spin_box.setSuffix(SongEditor::tr(suffix));
  QObject::connect(&spin_box, &QDoubleSpinBox::valueChanged, &song_editor,
                   value_action);
  spin_box.setValue(starting_value);
  controls_form.addRow(SongEditor::tr(label), &spin_box);
}

SongEditor::SongEditor()
    : player(Player(*this)), current_folder(QStandardPaths::writableLocation(
                                 QStandardPaths::DocumentsLocation)),
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

  auto &controls = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QFrame(this));
  controls.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  auto &dock_widget = *(new QDockWidget(SongEditor::tr("Controls"), this));

  auto &menu_bar = get_reference(menuBar());

  auto &file_menu = *(new QMenu(SongEditor::tr("&File"), this));

  add_menu_action(
      *this, file_menu, open_action,
      [this]() {
        if (undo_stack.isClean() || verify_discard_changes(*this)) {
          auto &dialog = make_file_dialog(
              *this, SongEditor::tr("Open — Justly"), "JSON file (*.json)",
              QFileDialog::AcceptOpen, ".json", QFileDialog::ExistingFile);
          if (dialog.exec() != 0) {
            reference_open_file(*this, get_selected_file(*this, dialog));
          }
        }
      },
      QKeySequence::Open);

  file_menu.addSeparator();

  add_menu_action(
      *this, file_menu, save_action,
      [this]() { reference_safe_as_file(*this, current_file); },
      QKeySequence::Save, false);

  add_menu_action(
      *this, file_menu,
      *(new QAction(SongEditor::tr("&Save As..."), &file_menu)),
      [this]() {
        auto &dialog = make_file_dialog(
            *this, SongEditor::tr("Save As — Justly"), "JSON file (*.json)",
            QFileDialog::AcceptSave, ".json", QFileDialog::AnyFile);

        if (dialog.exec() != 0) {
          reference_safe_as_file(*this, get_selected_file(*this, dialog));
        }
      },
      QKeySequence::SaveAs);

  add_menu_action(
      *this, file_menu,
      *(new QAction(SongEditor::tr("&Export recording"), &file_menu)),
      [this]() {
        auto &dialog = make_file_dialog(
            *this, SongEditor::tr("Export — Justly"), "WAV file (*.wav)",
            QFileDialog::AcceptSave, ".wav", QFileDialog::AnyFile);
        dialog.setLabelText(QFileDialog::Accept, "Export");
        if (dialog.exec() != 0) {
          export_song_to_file(player, song, get_selected_file(*this, dialog));
        }
      },
      QKeySequence::StandardKey());

  menu_bar.addMenu(&file_menu);

  auto &edit_menu = *(new QMenu(SongEditor::tr("&Edit"), this));
  auto &undo_action = get_reference(undo_stack.createUndoAction(&edit_menu));
  undo_action.setShortcuts(QKeySequence::Undo);
  edit_menu.addAction(&undo_action);

  auto &redo_action = get_reference(undo_stack.createRedoAction(&edit_menu));
  redo_action.setShortcuts(QKeySequence::Redo);
  edit_menu.addAction(&redo_action);

  edit_menu.addSeparator();

  add_menu_action(
      *this, edit_menu, cut_action,
      [this]() {
        copy_selection(*this);
        delete_cells(*this);
      },
      QKeySequence::Cut);

  add_menu_action(
      *this, edit_menu, copy_action, [this]() { copy_selection(*this); },
      QKeySequence::Copy);

  auto &paste_menu = *(new QMenu(SongEditor::tr("&Paste"), &edit_menu));
  edit_menu.addMenu(&paste_menu);

  add_menu_action(
      *this, paste_menu, paste_over_action,
      [this]() {
        switch (current_model_type) {
        case chords_type:
          paste_cells_template(*this, chords_model);
          return;
        case pitched_notes_type:
          paste_cells_template(*this, pitched_notes_model);
          return;
        case unpitched_notes_type:
          paste_cells_template(*this, unpitched_notes_model);
          return;
        default:
          Q_ASSERT(false);
          return;
        }
      },
      QKeySequence::Paste, false);

  add_menu_action(
      *this, paste_menu, paste_into_action,
      [this]() { paste_insert(*this, 0); }, QKeySequence::StandardKey());

  add_menu_action(
      *this, paste_menu, paste_after_action,
      [this]() {
        paste_insert(*this, get_only_range(table_view).bottom() + 1);
      },
      QKeySequence::StandardKey(), false);

  edit_menu.addSeparator();

  auto &insert_menu = *(new QMenu(SongEditor::tr("&Insert"), &edit_menu));

  edit_menu.addMenu(&insert_menu);

  add_menu_action(
      *this, insert_menu, insert_after_action,
      [this]() {
        insert_model_row(*this, get_only_range(table_view).bottom() + 1);
      },
      QKeySequence::InsertLineSeparator, false);

  add_menu_action(
      *this, insert_menu, insert_into_action,
      [this]() { insert_model_row(*this, 0); }, QKeySequence::AddTab);

  add_menu_action(
      *this, edit_menu, delete_action, [this]() { delete_cells(*this); },
      QKeySequence::Delete, false);

  add_menu_action(
      *this, edit_menu, remove_rows_action,
      [this]() {
        switch (current_model_type) {
        case chords_type:
          remove_rows_template(*this, chords_model);
          return;
        case pitched_notes_type:
          remove_rows_template(*this, pitched_notes_model);
          return;
        case unpitched_notes_type:
          remove_rows_template(*this, unpitched_notes_model);
          return;
        default:
          Q_ASSERT(false);
          break;
        }
      },
      QKeySequence::DeleteStartOfWord, false);

  edit_menu.addSeparator();

  add_menu_action(
      *this, edit_menu, back_to_chords_action,
      [this]() {
        add_edit_children_or_back(*this, current_chord_number,
                                  current_model_type == pitched_notes_type,
                                  true);
      },
      QKeySequence::Back, false);

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

  add_menu_action(
      *this, play_menu, play_action,
      [this]() {
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
            play_notes(player, current_chord_number, chord.pitched_notes,
                       first_row_number, number_of_rows);
          } else {
            play_notes(player, current_chord_number, chord.unpitched_notes,
                       first_row_number, number_of_rows);
          }
        }
      },
      QKeySequence::Print, false);

  add_menu_action(
      *this, play_menu, stop_playing_action, [this]() { stop_playing(player); },
      QKeySequence::Cancel);

  menu_bar.addMenu(&play_menu);

  auto &controls_form = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QFormLayout(&controls));

  add_control(*this, controls_form, "&Gain:", gain_editor, DEFAULT_GAIN, 0,
              MAX_GAIN, GAIN_STEP, 1, "/10", [this](double new_value) {
                set_double(*this, gain_editor, gain_id,
                           reference_get_gain(*this), new_value);
              });
  add_control(*this, controls_form, "Starting &key:", starting_key_editor,
              song.starting_key, MIN_STARTING_KEY, MAX_STARTING_KEY, 1, 0,
              " hz", [this](double new_value) {
                set_double(*this, starting_key_editor, starting_key_id,
                           song.starting_key, new_value);
              });
  add_control(
      *this, controls_form, "Starting &velocity:", starting_velocity_editor,
      song.starting_velocity, 0, MAX_VELOCITY, 1, 0, "/127",
      [this](double new_value) {
        set_double(*this, starting_velocity_editor, starting_velocity_id,
                   song.starting_velocity, new_value);
      });

  add_control(*this, controls_form, "Starting &tempo:", starting_tempo_editor,
              song.starting_tempo, MIN_STARTING_TEMPO, MAX_STARTING_TEMPO, 1, 0,
              " bpm", [this](double new_value) {
                set_double(*this, starting_tempo_editor, starting_tempo_id,
                           song.starting_tempo, new_value);
              });

  set_model(*this, chords_model);

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

  get_reference(table_view.horizontalHeader())
      .setSectionResizeMode(QHeaderView::ResizeToContents);

  table_view.setMouseTracking(true);

  connect(&table_view, &QAbstractItemView::doubleClicked, this,
          [this](const QModelIndex &index) {
            if (current_model_type == chords_type) {
              auto column = index.column();
              auto is_pitched = column == chord_pitched_notes_column;
              if (is_pitched || (column == chord_unpitched_notes_column)) {
                add_edit_children_or_back(*this, index.row(), is_pitched,
                                          false);
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

  resize(get_reference(QGuiApplication::primaryScreen())
             .availableGeometry()
             .size());

  undo_stack.clear();
  undo_stack.setClean();
}

SongEditor::~SongEditor() { undo_stack.disconnect(); }

auto reference_get_gain(const SongEditor &song_editor) -> double {
  return fluid_synth_get_gain(song_editor.player.synth_pointer);
}

void SongEditor::closeEvent(QCloseEvent *close_event_pointer) {
  if (!undo_stack.isClean() && !verify_discard_changes(*this)) {
    get_reference(close_event_pointer).ignore();
    return;
  }
  QMainWindow::closeEvent(close_event_pointer);
}

void reference_open_file(SongEditor &song_editor, const QString &filename) {
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

  static auto song_validator = []() {
    nlohmann::json song_schema(
        {{"gain", get_number_schema("number", "the gain (speaker volume)", 0,
                                    MAX_GAIN)},
         {"starting_key",
          get_number_schema("number", "the starting key, in Hz",
                            MIN_STARTING_KEY, MAX_STARTING_KEY)},
         {"starting_tempo",
          get_number_schema("number", "the starting tempo, in bpm",
                            MIN_STARTING_TEMPO, MAX_STARTING_TEMPO)},
         {"starting_velocity",
          get_number_schema("number", "the starting velocity (note force)", 0,
                            MAX_VELOCITY)}});
    add_row_array_schema<Chord>(song_schema);
    return make_validator(
        "Song", get_required_object_schema("A Justly song in JSON format",
                                           nlohmann::json({
                                               "starting_key",
                                               "starting_tempo",
                                               "starting_velocity",
                                           }),
                                           song_schema));
  }();
  try {
    song_validator.validate(json_song);
  } catch (const std::exception &error) {
    QMessageBox::warning(&song_editor, SongEditor::tr("Schema error"),
                         error.what());
    return;
  }

  set_double_from_json(json_song, song_editor.gain_editor, "gain");
  set_double_from_json(json_song, song_editor.starting_key_editor,
                       "starting_key");
  set_double_from_json(json_song, song_editor.starting_velocity_editor,
                       "starting_velocity");
  set_double_from_json(json_song, song_editor.starting_tempo_editor,
                       "starting_tempo");

  if (!chords.empty()) {
    chords_model.remove_rows(0, static_cast<int>(chords.size()));
  }

  if (json_song.contains("chords")) {
    chords_model.insert_json_rows(0, json_song["chords"]);
  }

  song_editor.current_file = filename;

  undo_stack.clear();
  undo_stack.setClean();
}

void reference_safe_as_file(SongEditor &song_editor, const QString &filename) {
  const auto &song = song_editor.song;

  std::ofstream file_io(filename.toStdString().c_str());

  nlohmann::json json_song;
  json_song["gain"] = reference_get_gain(song_editor);
  json_song["starting_key"] = song.starting_key;
  json_song["starting_tempo"] = song.starting_tempo;
  json_song["starting_velocity"] = song.starting_velocity;

  add_rows_to_json(json_song, song.chords, "chords");

  file_io << std::setw(4) << json_song;
  file_io.close();
  song_editor.current_file = filename;

  song_editor.undo_stack.setClean();
}
