#include "justly/ChordsModel.hpp"

#include <QAbstractItemModel>
#include <QByteArray>
#include <QClipboard>
#include <QGuiApplication>
#include <QItemSelectionModel>
#include <QList>
#include <QMessageBox>
#include <QMimeData>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <QUndoStack>
#include <QVariant>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
#include <algorithm>
#include <cstddef>
#include <exception>
#include <iomanip>
#include <iterator>
// IWYU pragma: no_include <map>
#include <memory>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "changes/CellChange.hpp"
#include "changes/CellChanges.hpp"
#include "changes/InsertChords.hpp"
#include "changes/InsertNotes.hpp"
#include "changes/RemoveChords.hpp"
#include "changes/RemoveNotes.hpp"
#include "justly/CellIndex.hpp"
#include "justly/Chord.hpp"
#include "justly/Note.hpp"
#include "justly/NoteChord.hpp"
#include "justly/NoteChordField.hpp"
#include "justly/Rational.hpp"
#include "justly/TreeLevel.hpp"
#include "other/private.hpp"

const auto CHORDS_MIME = "application/json+chords";
const auto NOTES_MIME = "application/json+notes";
const auto TEMPLATES_MIME = "application/json+templates";

auto get_column_name(NoteChordField note_chord_field) -> QString {
  switch (note_chord_field) {
  case type_column:
    return QWidget::tr("Type");
  case interval_column:
    return QWidget::tr("Interval");
  case beats_column:
    return QWidget::tr("Beats");
  case volume_ratio_column:
    return QWidget::tr("Volume ratio");
  case tempo_ratio_column:
    return QWidget::tr("Tempo ratio");
  case words_column:
    return QWidget::tr("Words");
  case instrument_column:
    return QWidget::tr("Instrument");
  default:
    Q_ASSERT(false);
    return {};
  }
}

[[nodiscard]] auto to_cell_index(const QModelIndex &index) {
  return CellIndex(to_size_t(index.row()), to_note_chord_field(index.column()),
                   index.parent().row());
}

[[nodiscard]] auto is_rows(const QItemSelection &selection) {
  Q_ASSERT(!selection.empty());
  return selection[0].left() == type_column;
}

[[nodiscard]] auto row_less_than_equals(int row_1, int row_2, int column_1,
                                        int column_2) -> bool {
  if (row_1 == row_2) {
    return column_2 > column_1;
  }
  return row_2 > row_1;
}

[[nodiscard]] auto index_less_than_equals(const QModelIndex &index_1,
                                          const QModelIndex &index_2) {
  auto index_1_level = get_level(index_1);
  auto index_2_level = get_level(index_2);
  auto index_1_row = index_1.row();
  auto index_2_row = index_2.row();

  auto index_1_column = index_1.column();
  auto index_2_column = index_2.column();

  if (index_1_level == chord_level) {
    if (index_2_level == chord_level) {
      return row_less_than_equals(index_1_row, index_2_row, index_1_column,
                                  index_2_column);
    }
    if (index_2_level == note_level) {
      // note is always below chord
      return index_2.parent().row() >= index_1_row;
    }
    Q_ASSERT(false);
    return false;
  }
  if (index_1_level == note_level) {
    auto index_1_parent_row = index_1.parent().row();
    if (index_2_level == chord_level) {
      // note is always below chord
      return index_2_row > index_1_parent_row;
    }
    if (index_2_level == note_level) {
      auto index_2_parent_row = index_2.parent().row();
      if (index_1_parent_row == index_2_parent_row) {
        return row_less_than_equals(index_1_row, index_2_row, index_1_column,
                                    index_2_column);
      }
      return index_2_parent_row > index_1_parent_row;
    }
    Q_ASSERT(false);
  }
  Q_ASSERT(false);
}

[[nodiscard]] auto
get_top_left_index(const QItemSelection &item_selection) -> QModelIndex {
  Q_ASSERT(!item_selection.empty());
  auto first_time = true;
  QModelIndex first_index;
  for (const auto &range : item_selection) {
    const auto &top_left_index = range.topLeft();
    if (first_time) {
      first_index = top_left_index;
      first_time = false;
    } else {
      if (index_less_than_equals(top_left_index, first_index)) {
        first_index = top_left_index;
      }
    }
  }
  return first_index;
};

[[nodiscard]] auto
get_bottom_right_index(const QItemSelection &item_selection) -> QModelIndex {
  Q_ASSERT(!item_selection.empty());
  auto first_time = true;
  QModelIndex last_index;
  for (const auto &range : item_selection) {
    const auto &bottom_right_index = range.bottomRight();
    if (first_time) {
      last_index = bottom_right_index;
      first_time = false;
    } else {
      if (index_less_than_equals(last_index, bottom_right_index)) {
        last_index = bottom_right_index;
      }
    }
  }
  return last_index;
};

auto copy_json(const nlohmann::json &copied, const QString &mime_type) {
  std::stringstream json_text;
  json_text << std::setw(4) << copied;
  copy_text(json_text.str(), mime_type);
}

auto get_level(const QModelIndex &index) -> TreeLevel {
  // root will be an invalid index
  return !index.isValid() ? root_level
         // chords have null parent pointers
         : index.internalPointer() == nullptr ? chord_level
                                              : note_level;
}

auto make_validator(const std::string &title, nlohmann::json json)
    -> nlohmann::json_schema::json_validator {
  json["$schema"] = "http://json-schema.org/draft-07/schema#";
  json["title"] = title;
  return {json};
}

auto validate_json(QWidget *parent_pointer, const nlohmann::json &copied,
                   const nlohmann::json_schema::json_validator &validator)
    -> bool {
  try {
    validator.validate(copied);
    return true;
  } catch (const std::exception &error) {
    std::stringstream error_message;
    QMessageBox::warning(parent_pointer, QWidget::tr("Schema error"),
                         error.what());
    return false;
  }
}

void copy_text(const std::string &text, const QString &mime_type) {
  auto *new_data_pointer = std::make_unique<QMimeData>().release();

  Q_ASSERT(new_data_pointer != nullptr);
  new_data_pointer->setData(mime_type, text.c_str());

  auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  clipboard_pointer->setMimeData(new_data_pointer);
}

void ChordsModel::check_chord_number(size_t chord_number) const {
  Q_ASSERT(chord_number < chords.size());
}

void ChordsModel::check_chord_number_end(size_t chord_number) const {
  Q_ASSERT(chord_number <= chords.size());
}

auto ChordsModel::get_const_chord(size_t chord_number) const -> const Chord & {
  check_chord_number(chord_number);
  return chords[chord_number];
}

auto ChordsModel::get_chord(size_t chord_number) -> Chord & {
  check_chord_number(chord_number);
  return chords[chord_number];
}

[[nodiscard]] auto ChordsModel::get_const_note_chord_pointer(
    const QModelIndex &index) const -> const NoteChord * {
  auto child_number = to_size_t(index.row());
  if (get_level(index) == chord_level) {
    return &get_const_chord(child_number);
  }
  return &get_const_chord(to_size_t(index.parent().row()))
              .get_const_note(child_number);
}

auto ChordsModel::get_number_of_rows_left(size_t first_chord_number) const
    -> size_t {
  size_t number_of_rows_left = 0;
  for (size_t chord_number = first_chord_number; chord_number < chords.size();
       chord_number = chord_number + 1) {
    number_of_rows_left =
        number_of_rows_left + 1 + chords[chord_number].notes.size();
  }
  return number_of_rows_left;
}

auto ChordsModel::get_bottom_right_index_from_chord(
    size_t chord_number, NoteChordField note_chord_field,
    size_t number_of_rows) const -> QModelIndex {
  // subtract 1 extra for first chord
  number_of_rows = number_of_rows - 1;
  while (number_of_rows > 0) {
    auto number_of_notes = get_const_chord(chord_number).notes.size();
    if (number_of_rows <= number_of_notes) {
      // subtract 1 for 0 indexing
      return get_note_index(chord_number, number_of_rows - 1, note_chord_field);
    }
    number_of_rows = number_of_rows - number_of_notes;

    chord_number = chord_number + 1;
    // subtract 1 extra for new chord
    number_of_rows = number_of_rows - 1;
  }
  return get_chord_index(chord_number, note_chord_field);
}

auto ChordsModel::parse_clipboard(const QString &mime_type) -> bool {
  const auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  const auto *mime_data_pointer = clipboard_pointer->mimeData();
  Q_ASSERT(mime_data_pointer != nullptr);

  if (!mime_data_pointer->hasFormat(mime_type)) {
    auto formats = mime_data_pointer->formats();
    Q_ASSERT(!(formats.empty()));
    QString message;
    QTextStream stream(&message);
    stream << tr("Cannot paste MIME type \"") << formats[0]
           << tr("\" into destination needing MIME type \"") << mime_type;
    QMessageBox::warning(parent_pointer, tr("MIME type error"), message);
    return false;
  }
  const auto &copied_text = mime_data_pointer->data(mime_type).toStdString();
  try {
    clipboard = nlohmann::json::parse(copied_text);
  } catch (const nlohmann::json::parse_error &parse_error) {
    QMessageBox::warning(parent_pointer, tr("Parsing error"),
                         parse_error.what());
    return false;
  }
  return true;
}

void ChordsModel::add_cell_change(const QModelIndex &index,
                                  const QVariant &new_value) {
  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->push(
      std::make_unique<CellChange>(this, to_cell_index(index),
                                   data(index, Qt::EditRole), new_value)
          .release());
}

void ChordsModel::add_cell_changes(
    const QModelIndex &top_left_index, const QModelIndex &bottom_right_index,
    const std::vector<NoteChord> &old_note_chords,
    const std::vector<NoteChord> &new_note_chords) {
  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->push(std::make_unique<CellChanges>(
                               this, to_cell_index(top_left_index),
                               to_note_chord_field(bottom_right_index.column()),
                               old_note_chords, new_note_chords)
                               .release());
}

void ChordsModel::copy_note_chords_from_chord(
    size_t first_chord_number, const QModelIndex &bottom_right_index,
    std::vector<NoteChord> *note_chords_pointer) const {
  Q_ASSERT(note_chords_pointer != nullptr);

  auto ends_on_chord = get_level(bottom_right_index) == chord_level;
  auto last_chord_number = ends_on_chord
                               ? to_size_t(bottom_right_index.row())
                               : to_size_t(bottom_right_index.parent().row());
  for (auto chord_number = first_chord_number; chord_number < last_chord_number;
       chord_number = chord_number + 1) {
    const auto &chord = get_const_chord(chord_number);
    note_chords_pointer->emplace_back(chord);
    chord.copy_notes_to(0, chord.notes.size(), note_chords_pointer);
  }
  const auto &last_chord = get_const_chord(last_chord_number);
  note_chords_pointer->emplace_back(last_chord);
  if (!ends_on_chord) {
    last_chord.copy_notes_to(0, bottom_right_index.row() + 1,
                             note_chords_pointer);
  }
}

auto ChordsModel::copy_note_chords(const QModelIndex &top_left_index,
                                   const QModelIndex &bottom_right_index) const
    -> std::vector<NoteChord> {
  std::vector<NoteChord> note_chords;
  if (get_level(top_left_index) == chord_level) {
    copy_note_chords_from_chord(to_size_t(top_left_index.row()),
                                bottom_right_index, &note_chords);
  } else {
    auto first_chord_number = to_size_t(top_left_index.parent().row());
    const auto &first_chord = get_const_chord(first_chord_number);
    auto first_note_number = top_left_index.row();
    if (get_level(bottom_right_index) == note_level) {
      auto bottom_right_chord_number =
          to_size_t(bottom_right_index.parent().row());
      if (first_chord_number == bottom_right_chord_number) {
        auto bottom_right_note_number = to_size_t(bottom_right_index.row());
        first_chord.copy_notes_to(
            first_note_number, bottom_right_note_number - first_note_number + 1,
            &note_chords);
        return note_chords;
      }
    }
    first_chord.copy_notes_to(first_note_number,
                              first_chord.notes.size() - first_note_number,
                              &note_chords);
    copy_note_chords_from_chord(first_chord_number + 1, bottom_right_index,
                                &note_chords);
  }
  return note_chords;
}

auto ChordsModel::replace_note_cells(const CellIndex &top_left_cell_index,
                                     NoteChordField right_field,
                                     const std::vector<NoteChord> &note_chords,
                                     size_t first_note_chord_number) -> size_t {
  auto first_child_number = top_left_cell_index.child_number;
  auto notes_size =
      get_chord(to_size_t(top_left_cell_index.parent_number)).notes.size();
  auto write_number = std::min({note_chords.size() - first_note_chord_number,
                                notes_size - first_child_number});
  auto chord_number = to_size_t(top_left_cell_index.parent_number);
  auto first_note_number = top_left_cell_index.child_number;
  auto left_field = top_left_cell_index.note_chord_field;
  get_chord(chord_number)
      .replace_note_cells(first_note_number, left_field, right_field,
                          note_chords, first_note_chord_number, write_number);
  emit dataChanged(get_note_index(chord_number, first_note_number, left_field),
                   get_note_index(chord_number,
                                  first_note_number + write_number - 1,
                                  right_field),
                   {Qt::DisplayRole, Qt::EditRole});
  return write_number;
};

ChordsModel::ChordsModel(QUndoStack *undo_stack_pointer_input,
                         QWidget *parent_pointer_input)
    : QAbstractItemModel(parent_pointer_input),
      parent_pointer(parent_pointer_input),
      undo_stack_pointer(undo_stack_pointer_input) {}

auto ChordsModel::get_chord_index(
    size_t chord_number, NoteChordField note_chord_field) const -> QModelIndex {
  return createIndex(static_cast<int>(chord_number), note_chord_field, nullptr);
}

auto ChordsModel::get_note_index(size_t chord_number, size_t note_number,
                                 NoteChordField note_chord_field) const
    -> QModelIndex {
  return createIndex(static_cast<int>(note_number), note_chord_field,
                     &get_const_chord(chord_number));
}

auto ChordsModel::rowCount(const QModelIndex &parent_index) const -> int {
  auto parent_level = get_level(parent_index);
  if (parent_level == root_level) {
    return static_cast<int>(chords.size());
  }
  // only nest into the symbol cell
  if (parent_level == chord_level && parent_index.column() == type_column) {
    return static_cast<int>(
        get_const_chord(to_size_t(parent_index.row())).notes.size());
  }
  // notes and non-symbol chord cells have no children
  return 0;
}

auto ChordsModel::columnCount(const QModelIndex & /*parent*/) const -> int {
  return NOTE_CHORD_COLUMNS;
}

// get the parent index
auto ChordsModel::parent(const QModelIndex &index) const -> QModelIndex {
  const auto *internal_pointer = index.internalPointer();
  // chords have null parents
  if (internal_pointer == nullptr) {
    return {};
  }
  return createIndex(
      static_cast<int>(std::distance(
          chords.data(), static_cast<const Chord *>(internal_pointer))),
      type_column, nullptr);
}

auto ChordsModel::index(int child_number, int column,
                        const QModelIndex &parent_index) const -> QModelIndex {
  auto unsigned_child_number = to_size_t(child_number);
  if (get_level(parent_index) == root_level) {
    return get_chord_index(unsigned_child_number, to_note_chord_field(column));
  }
  return get_note_index(to_size_t(parent_index.row()), unsigned_child_number,
                        to_note_chord_field(column));
}

auto ChordsModel::headerData(int section, Qt::Orientation orientation,
                             int role) const -> QVariant {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    return get_column_name(to_note_chord_field(section));
  }
  // no horizontal headers
  // no headers for other roles
  return {};
}

auto ChordsModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  auto selectable = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  if (to_note_chord_field(index.column()) == type_column) {
    return selectable;
  }
  return selectable | Qt::ItemIsEditable;
}

auto ChordsModel::data(const QModelIndex &index, int role) const -> QVariant {
  const auto *note_chord_pointer = get_const_note_chord_pointer(index);
  Q_ASSERT(note_chord_pointer != nullptr);
  return note_chord_pointer->data(to_note_chord_field(index.column()), role);
}

auto ChordsModel::setData(const QModelIndex &index, const QVariant &new_value,
                          int role) -> bool {
  if (role != Qt::EditRole) {
    return false;
  }
  add_cell_change(index, new_value);
  return true;
}

auto ChordsModel::insertRows(int first_child_number, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  Q_ASSERT(undo_stack_pointer != nullptr);

  if (get_level(parent_index) == root_level) {
    Chord template_chord;
    if (first_child_number > 0) {
      template_chord.beats =
          get_const_chord(to_size_t(first_child_number - 1)).beats;
    }

    std::vector<Chord> new_chords;
    for (auto index = 0; index < number_of_children; index = index + 1) {
      new_chords.push_back(template_chord);
    }
    undo_stack_pointer->push(
        std::make_unique<InsertChords>(this, first_child_number, new_chords)
            .release());
  } else {
    auto chord_number = to_size_t(parent_index.row());
    const auto &parent_chord = get_const_chord(chord_number);

    Note template_note;

    if (first_child_number == 0) {
      template_note.beats = parent_chord.beats;
      template_note.words = parent_chord.words;
    } else {
      const auto &previous_note =
          parent_chord.get_const_note(to_size_t(first_child_number - 1));

      template_note.beats = previous_note.beats;
      template_note.volume_ratio = previous_note.volume_ratio;
      template_note.tempo_ratio = previous_note.tempo_ratio;
      template_note.words = previous_note.words;
    }

    std::vector<Note> new_notes;
    for (auto index = 0; index < number_of_children; index = index + 1) {
      new_notes.push_back(template_note);
    }
    Q_ASSERT(undo_stack_pointer != nullptr);
    undo_stack_pointer->push(std::make_unique<InsertNotes>(this, chord_number,
                                                           first_child_number,
                                                           new_notes)
                                 .release());
  }
  return true;
}

auto ChordsModel::removeRows(int first_child_number, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  auto unsigned_first_child_number = to_size_t(first_child_number);

  auto end_number = first_child_number + number_of_children;
  auto unsigned_end_number = to_size_t(end_number);

  Q_ASSERT(undo_stack_pointer != nullptr);

  if (get_level(parent_index) == root_level) {
    check_chord_number(unsigned_first_child_number);
    check_chord_number_end(unsigned_end_number);

    undo_stack_pointer->push(
        std::make_unique<RemoveChords>(
            this, unsigned_first_child_number,
            std::vector<Chord>(chords.cbegin() + first_child_number,
                               chords.cbegin() + end_number))
            .release());
  } else {
    auto chord_number = to_size_t(parent_index.row());
    const auto &chord = get_const_chord(chord_number);
    undo_stack_pointer->push(
        std::make_unique<RemoveNotes>(
            this, chord_number, unsigned_first_child_number,
            chord.copy_notes(unsigned_first_child_number, unsigned_end_number))
            .release());
  }
  return true;
}

void ChordsModel::set_cell(const CellIndex &cell_index,
                           const QVariant &new_value) {
  auto child_number = cell_index.child_number;
  auto note_chord_field = cell_index.note_chord_field;
  auto parent_number = cell_index.parent_number;

  if (parent_number == -1) {
    get_chord(child_number).setData(note_chord_field, new_value);
    auto index = get_chord_index(child_number, note_chord_field);
    emit dataChanged(index, index,
                     {Qt::DisplayRole, Qt::EditRole, Qt::EditRole});
  } else {
    auto chord_number = to_size_t(parent_number);
    get_chord(chord_number)
        .get_note(child_number)
        .setData(note_chord_field, new_value);
    auto index = get_note_index(chord_number, child_number, note_chord_field);
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
  }
}

void ChordsModel::replace_cells(const CellIndex &top_left_cell_index,
                                NoteChordField right_field,
                                const std::vector<NoteChord> &note_chords) {
  auto first_child_number = top_left_cell_index.child_number;
  auto left_field = top_left_cell_index.note_chord_field;
  size_t current_note_chord_number = 0;
  size_t current_chord_number = 0;
  if (top_left_cell_index.parent_number == -1) {
    current_chord_number = first_child_number;
  } else {
    current_note_chord_number =
        current_note_chord_number +
        replace_note_cells(top_left_cell_index, right_field, note_chords);
    current_chord_number = to_size_t(top_left_cell_index.parent_number + 1);
  }
  auto number_of_note_chords = note_chords.size();
  while (current_note_chord_number < number_of_note_chords) {
    auto &chord = get_chord(current_chord_number);
    chord.replace_cells(left_field, right_field,
                        note_chords[current_note_chord_number]);
    emit dataChanged(get_chord_index(current_chord_number, left_field),
                     get_chord_index(current_chord_number, right_field),
                     {Qt::DisplayRole, Qt::EditRole});
    current_note_chord_number = current_note_chord_number + 1;
    if (current_note_chord_number >= number_of_note_chords) {
      break;
    }
    current_note_chord_number =
        current_note_chord_number +
        replace_note_cells(
            CellIndex(0, left_field, static_cast<int>(current_chord_number)),
            right_field, note_chords, current_note_chord_number);
    current_chord_number = current_chord_number + 1;
  }
};

auto ChordsModel::copy_chords_to_json(size_t first_chord_number,
                                      size_t number_of_chords) const
    -> nlohmann::json {
  nlohmann::json json_chords;

  check_chord_number(first_chord_number);

  auto end_number = first_chord_number + number_of_chords;
  check_chord_number_end(end_number);

  std::transform(chords.cbegin() + static_cast<int>(first_chord_number),
                 chords.cbegin() + static_cast<int>(end_number),
                 std::back_inserter(json_chords),
                 [](const Chord &chord) { return chord.json(); });
  return json_chords;
}

void ChordsModel::paste_rows(size_t first_child_number,
                             const QModelIndex &parent_index) {
  Q_ASSERT(undo_stack_pointer != nullptr);
  if (get_level(parent_index) == root_level) {
    if (!parse_clipboard(CHORDS_MIME)) {
      return;
    }

    static const nlohmann::json_schema::json_validator chords_validator =
        make_validator("Chords", get_chords_schema());
    if (!validate_json(parent_pointer, clipboard, chords_validator)) {
      return;
    }

    std::vector<Chord> chords;
    std::transform(
        clipboard.cbegin(), clipboard.cend(), std::back_inserter(chords),
        [](const nlohmann::json &json_chord) { return Chord(json_chord); });

    undo_stack_pointer->push(
        std::make_unique<InsertChords>(this, first_child_number, chords)
            .release());

  } else {
    if (!parse_clipboard(NOTES_MIME)) {
      return;
    }

    static const nlohmann::json_schema::json_validator notes_validator =
        make_validator("Notes", get_notes_schema());
    if (!validate_json(parent_pointer, clipboard, notes_validator)) {
      return;
    }

    std::vector<Note> notes;
    std::transform(
        clipboard.cbegin(), clipboard.cend(), std::back_inserter(notes),
        [](const nlohmann::json &json_note) { return Note(json_note); });
    undo_stack_pointer->push(
        std::make_unique<InsertNotes>(this, to_size_t(parent_index.row()),
                                      first_child_number, notes)
            .release());
  }
}

void ChordsModel::insert_chords(size_t first_chord_number,
                                const std::vector<Chord> &new_chords) {
  auto int_first_chord_number = static_cast<int>(first_chord_number);

  check_chord_number_end(first_chord_number);

  beginInsertRows(QModelIndex(), int_first_chord_number,
                  static_cast<int>(first_chord_number + new_chords.size()) - 1);
  chords.insert(chords.begin() + int_first_chord_number, new_chords.begin(),
                new_chords.end());
  endInsertRows();
}

void ChordsModel::insert_json_chords(size_t first_chord_number,
                                     const nlohmann::json &json_chords) {
  beginInsertRows(QModelIndex(), static_cast<int>(first_chord_number),
                  static_cast<int>(first_chord_number + json_chords.size()) -
                      1);
  std::transform(
      json_chords.cbegin(), json_chords.cend(),
      std::inserter(chords,
                    chords.begin() + static_cast<int>(first_chord_number)),
      [](const nlohmann::json &json_chord) { return Chord(json_chord); });
  endInsertRows();
}

void ChordsModel::remove_chords(size_t first_chord_number,
                                size_t number_of_chords) {
  auto end_number = first_chord_number + number_of_chords;
  auto int_first_chord_number = static_cast<int>(first_chord_number);
  auto int_end_number = static_cast<int>(end_number);

  beginRemoveRows(QModelIndex(), int_first_chord_number, int_end_number - 1);
  check_chord_number(first_chord_number);
  check_chord_number_end(end_number);
  chords.erase(chords.begin() + int_first_chord_number,
               chords.begin() + int_end_number);
  endRemoveRows();
}

void ChordsModel::delete_all_chords() {
  if (!chords.empty()) {
    remove_chords(0, chords.size());
  }
}

void ChordsModel::insert_notes(size_t chord_number, size_t first_note_number,
                               const std::vector<Note> &new_notes) {
  beginInsertRows(get_chord_index(chord_number),
                  static_cast<int>(first_note_number),
                  static_cast<int>(first_note_number + new_notes.size()) - 1);
  get_chord(chord_number).insert_notes(first_note_number, new_notes);
  endInsertRows();
};

void ChordsModel::insert_json_notes(size_t chord_number,
                                    size_t first_note_number,
                                    const nlohmann::json &json_notes) {
  beginInsertRows(get_chord_index(chord_number),
                  static_cast<int>(first_note_number),
                  static_cast<int>(first_note_number + json_notes.size()) - 1);
  get_chord(chord_number).insert_json_notes(first_note_number, json_notes);
  endInsertRows();
}

void ChordsModel::remove_notes(size_t chord_number, size_t first_note_number,
                               size_t number_of_notes) {
  beginRemoveRows(get_chord_index(chord_number),
                  static_cast<int>(first_note_number),
                  static_cast<int>(first_note_number + number_of_notes) - 1);
  get_chord(chord_number).remove_notes(first_note_number, number_of_notes);
  endRemoveRows();
}

void ChordsModel::copy_selected(const QItemSelection &selection) const {
  auto top_left_index = get_top_left_index(selection);
  auto bottom_right_index = get_bottom_right_index(selection);
  if (is_rows(selection)) {
    auto number_of_children =
        bottom_right_index.row() - top_left_index.row() + 1;
    if (get_level(top_left_index) == chord_level) {
      copy_json(copy_chords_to_json(to_size_t(top_left_index.row()),
                                    number_of_children),
                CHORDS_MIME);
    } else {
      copy_json(get_const_chord(to_size_t(top_left_index.parent().row()))
                    .copy_notes_to_json(to_size_t(top_left_index.row()),
                                        number_of_children),
                NOTES_MIME);
    }
  } else {
    const auto note_chords =
        copy_note_chords(top_left_index, bottom_right_index);
    nlohmann::json json_note_chords;
    std::transform(
        note_chords.cbegin(), note_chords.cend(),
        std::back_inserter(json_note_chords),
        [](const NoteChord &note_chord) { return note_chord.json(); });
    copy_json(nlohmann::json({{"left_field", top_left_index.column()},
                              {"right_field", bottom_right_index.column()},
                              {"note_chords", json_note_chords}}),
              TEMPLATES_MIME);
  }
}

void ChordsModel::paste_cells_or_after(const QItemSelection &selection) {
  auto top_left_index = get_top_left_index(selection);
  auto top_left_row = to_size_t(top_left_index.row());
  auto top_left_parent = top_left_index.parent();
  if (is_rows(selection)) {
    paste_rows(top_left_row + 1, top_left_parent);
  } else {
    if (!parse_clipboard(TEMPLATES_MIME)) {
      return;
    }

    static const nlohmann::json_schema::json_validator templates_validator =
        make_validator(
            "NoteChord Templates",
            nlohmann::json(
                {{"description", "NoteChord templates"},
                 {"type", "object"},
                 {"required", {"left_field", "right_field", "note_chords"}},
                 {"properties",
                  {{"left_field",
                    {{"type", "number"},
                     {"description", "left NoteChord field"},
                     {"minimum", type_column},
                     {"maximum", words_column}}},
                   {"right_field",
                    {{"type", "number"},
                     {"description", "right NoteChord field"},
                     {"minimum", type_column},
                     {"maximum", words_column}}},
                   {"note_chords",
                    {{"type", "array"},
                     {"description", "a list of NoteChord templates"},
                     {"items",
                      {{"type", "object"},
                       {"description", "NoteChord templates"},
                       {"properties", get_note_chord_fields_schema()}}}}}}}}));
    if (!(validate_json(parent_pointer, clipboard, templates_validator))) {
      return;
    }

    Q_ASSERT(clipboard.contains("left_field"));
    auto left_field_value = clipboard["left_field"];
    Q_ASSERT(left_field_value.is_number());
    auto left_field = to_note_chord_field(left_field_value.get<int>());

    auto left_paste_column = to_note_chord_field(top_left_index.column());

    if (left_field != left_paste_column) {
      QString message;
      QTextStream stream(&message);
      stream << tr("Destination left column ") << left_paste_column
             << tr(" doesn't match pasted left column ") << left_field;
      QMessageBox::warning(parent_pointer, tr("Column number error"), message);
      return;
    }
    Q_ASSERT(clipboard.contains("right_field"));
    auto right_field_value = clipboard["right_field"];
    Q_ASSERT(right_field_value.is_number());

    Q_ASSERT(clipboard.contains("note_chords"));
    auto &json_note_chords = clipboard["note_chords"];

    size_t number_of_rows_left = 0;
    auto number_of_note_chords = json_note_chords.size();
    if (get_level(top_left_index) == chord_level) {
      number_of_rows_left = get_number_of_rows_left(top_left_row);

    } else {
      auto chord_number = to_size_t(top_left_parent.row());
      number_of_rows_left = get_chord(chord_number).notes.size() -
                            top_left_row +
                            get_number_of_rows_left(chord_number + 1);
    }

    if (number_of_note_chords > number_of_rows_left) {
      QString message;
      QTextStream stream(&message);
      stream << tr("Pasted ") << number_of_note_chords << tr(" rows but only ")
             << number_of_rows_left << tr(" rows available");
      QMessageBox::warning(parent_pointer, tr("Row number error"), message);
      return;
    }

    std::vector<NoteChord> note_chords;
    std::transform(json_note_chords.cbegin(), json_note_chords.cend(),
                   std::back_inserter(note_chords),
                   [](const nlohmann::json &json_template) {
                     return NoteChord(json_template);
                   });

    auto number_of_rows = note_chords.size();
    QModelIndex bottom_right_index;
    auto top_child_number = to_size_t(top_left_index.row());
    auto right_field = to_note_chord_field(right_field_value.get<int>());
    if (get_level(top_left_index) == chord_level) {
      bottom_right_index = get_bottom_right_index_from_chord(
          top_child_number, right_field, number_of_rows);
    } else {
      auto chord_number = to_size_t(top_left_index.parent().row());
      auto number_of_notes = get_const_chord(chord_number).notes.size();
      auto number_of_notes_left = number_of_notes - top_child_number;
      bottom_right_index =
          number_of_rows <= number_of_notes_left
              ? get_note_index(chord_number,
                               top_child_number + number_of_rows - 1,
                               right_field)
              : get_bottom_right_index_from_chord(chord_number + 1, right_field,
                                                  number_of_rows -
                                                      number_of_notes_left);
    }
    add_cell_changes(top_left_index, bottom_right_index,
                     copy_note_chords(top_left_index, bottom_right_index),
                     note_chords);
  }
}

void ChordsModel::delete_selected(const QItemSelection &selection) {
  auto top_left_index = get_top_left_index(selection);
  auto bottom_right_index = get_bottom_right_index(selection);
  if (is_rows(selection)) {
    auto top_left_row = top_left_index.row();
    removeRows(top_left_row, bottom_right_index.row() - top_left_row + 1,
               top_left_index.parent());
  } else {
    auto old_note_chords = copy_note_chords(top_left_index, bottom_right_index);
    add_cell_changes(top_left_index, bottom_right_index, old_note_chords,
                     std::vector<NoteChord>(old_note_chords.size()));
  }
}
