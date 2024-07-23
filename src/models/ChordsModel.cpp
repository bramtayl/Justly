#include "justly/ChordsModel.hpp"

#include <QAbstractItemModel>
#include <QByteArray>
#include <QClipboard>
#include <QGuiApplication>
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

#include "changes/ChordCellChange.hpp"
#include "changes/ChordCellChanges.hpp"
#include "changes/InsertChords.hpp"
#include "changes/InsertNotes.hpp"
#include "changes/NoteCellChange.hpp"
#include "changes/NoteCellChanges.hpp"
#include "changes/RemoveChords.hpp"
#include "changes/RemoveNotes.hpp"
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

auto is_rows(const QItemSelection &selection) {
  Q_ASSERT(!selection.empty());
  return selection[0].left() == type_column;
}

void copy_json(const nlohmann::json &copied, const QString &mime_type) {
  std::stringstream json_text;
  json_text << std::setw(4) << copied;
  copy_text(json_text.str(), mime_type);
}

auto get_level(QModelIndex index) -> TreeLevel {
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

auto row_less_than_equals(int row_1, int row_2, int column_1,
                          int column_2) -> bool {
  if (row_2 > row_1) {
    return true;
  }
  if (row_1 == row_2) {
    return column_2 > column_1;
  }
  return false;
}

auto ChordsModel::index_less_than_equals(
    const QModelIndex &index_1, const QModelIndex &index_2) const -> bool {
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
      return parent(index_2).row() >= index_1_row;
    }
    Q_ASSERT(false);
    return false;
  }
  if (index_1_level == note_level) {
    auto index_1_parent_row = parent(index_1).row();
    if (index_2_level == chord_level) {
      // note is always below chord
      return index_2_row > index_1_parent_row;
    }
    if (index_2_level == note_level) {
      auto index_2_parent_row = parent(index_2).row();
      if (index_2_parent_row > index_1_parent_row) {
        return true;
      }
      if (index_1_parent_row == index_2_parent_row) {
        return row_less_than_equals(index_1_row, index_2_row, index_1_column,
                                    index_2_column);
      }
      return false;
    }
    Q_ASSERT(false);
  }
  Q_ASSERT(false);
}

auto ChordsModel::top_left_index(const QItemSelection &item_selection) const
    -> QModelIndex {
  Q_ASSERT(!item_selection.empty());
  auto first_time = true;
  QModelIndex first_index;
  for (const auto &range : item_selection) {
    const auto &top_left = range.topLeft();
    if (first_time) {
      first_index = top_left;
      first_time = false;
    } else {
      if (index_less_than_equals(top_left, first_index)) {
        first_index = top_left;
      }
    }
  }
  return first_index;
};

auto ChordsModel::bottom_right_index(const QItemSelection &item_selection) const
    -> QModelIndex {
  Q_ASSERT(!item_selection.empty());
  auto first_time = true;
  QModelIndex last_index;
  for (const auto &range : item_selection) {
    const auto &bottom_right = range.bottomRight();
    if (first_time) {
      last_index = bottom_right;
      first_time = false;
    } else {
      if (index_less_than_equals(last_index, bottom_right)) {
        last_index = bottom_right;
      }
    }
  }
  return last_index;
};

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
  return &get_const_chord(to_size_t(parent(index).row()))
              .get_const_note(child_number);
}

auto ChordsModel::get_number_of_additional_note_chords(
    size_t first_chord_number) const -> size_t {
  size_t number_of_additional_note_chords = 0;
  for (size_t chord_number = first_chord_number; chord_number < chords.size();
       chord_number = chord_number + 1) {
    number_of_additional_note_chords = number_of_additional_note_chords + 1 +
                                       chords[chord_number].notes.size();
  }
  return number_of_additional_note_chords;
}

auto ChordsModel::get_bottom_right_index_from_chord(
    size_t chord_number, NoteChordField note_chord_field,
    size_t skip_rows) const -> QModelIndex {
  while (true) {
    if (skip_rows == 0) {
      return get_chord_index(chord_number, note_chord_field);
    }
    skip_rows = skip_rows - 1;
    auto number_of_notes = get_const_chord(chord_number).notes.size();
    if (skip_rows < number_of_notes) {
      return get_note_index(chord_number, skip_rows, note_chord_field);
    }
    skip_rows = skip_rows - number_of_notes;
    chord_number = chord_number + 1;
  }
}

auto ChordsModel::get_bottom_right_index(
    const QModelIndex &first_row_index, NoteChordField note_chord_field,
    size_t skip_rows) const -> QModelIndex {
  if (get_level(first_row_index) == chord_level) {
    return get_bottom_right_index_from_chord(to_size_t(first_row_index.row()),
                                             note_chord_field, skip_rows);
  }
  auto chord_number = to_size_t(parent(first_row_index).row());
  auto number_of_notes = get_const_chord(chord_number).notes.size();
  auto note_number = to_size_t(first_row_index.row());
  auto number_of_notes_left = number_of_notes - note_number;
  if (skip_rows < number_of_notes_left) {
    return get_note_index(chord_number, note_number + skip_rows,
                          note_chord_field);
  }
  return get_bottom_right_index_from_chord(chord_number + 1, note_chord_field,
                                           skip_rows - number_of_notes_left);
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

  auto child_number = to_size_t(index.row());
  auto note_chord_field = to_note_chord_field(index.column());

  if (get_level(index) == chord_level) {
    auto old_value = data(index, Qt::EditRole);
    undo_stack_pointer->push(
        std::make_unique<ChordCellChange>(this, child_number, note_chord_field,
                                          data(index, Qt::EditRole), new_value)
            .release());
  } else {
    undo_stack_pointer->push(
        std::make_unique<NoteCellChange>(this, to_size_t(parent(index).row()),
                                         child_number, note_chord_field,
                                         data(index, Qt::EditRole), new_value)
            .release());
  }
}

void ChordsModel::add_cell_changes(
    const QModelIndex &top_left, const QModelIndex &bottom_right,
    const std::vector<NoteChord> &old_note_chord_templates,
    const std::vector<NoteChord> &new_note_chord_templates) {
  auto left_note_chord_field = to_note_chord_field(top_left.column());
  auto right_note_chord_field = to_note_chord_field(bottom_right.column());
  auto child_number = to_size_t(top_left.row());
  Q_ASSERT(undo_stack_pointer != nullptr);
  if (get_level(top_left) == chord_level) {
    undo_stack_pointer->push(
        std::make_unique<ChordCellChanges>(
            this, child_number, left_note_chord_field, right_note_chord_field,
            old_note_chord_templates, new_note_chord_templates)
            .release());
  } else {
    undo_stack_pointer->push(
        std::make_unique<NoteCellChanges>(
            this, to_size_t(parent(top_left).row()), child_number,
            left_note_chord_field, right_note_chord_field,
            old_note_chord_templates, new_note_chord_templates)
            .release());
  }
}

void ChordsModel::copy_note_chord_templates_from_chord(
    size_t first_chord_number, const QModelIndex &bottom_right,
    std::vector<NoteChord> *note_chords_pointer) const {
  Q_ASSERT(note_chords_pointer != nullptr);

  auto ends_on_chord = get_level(bottom_right) == chord_level;
  auto last_chord_number = ends_on_chord
                               ? to_size_t(bottom_right.row())
                               : to_size_t(parent(bottom_right).row());
  for (auto chord_number = first_chord_number; chord_number < last_chord_number;
       chord_number = chord_number + 1) {
    const auto &chord = get_const_chord(chord_number);
    note_chords_pointer->emplace_back(chord);
    chord.copy_notes_to(0, chord.notes.size(), note_chords_pointer);
  }
  const auto &last_chord = get_const_chord(last_chord_number);
  note_chords_pointer->emplace_back(last_chord);
  if (!ends_on_chord) {
    last_chord.copy_notes_to(0, bottom_right.row() + 1, note_chords_pointer);
  }
}

auto ChordsModel::copy_note_chord_templates(
    const QModelIndex &top_left,
    const QModelIndex &bottom_right) const -> std::vector<NoteChord> {
  std::vector<NoteChord> note_chord_templates;
  if (get_level(top_left) == chord_level) {
    copy_note_chord_templates_from_chord(to_size_t(top_left.row()),
                                         bottom_right, &note_chord_templates);
  } else {
    auto first_chord_number = to_size_t(parent(top_left).row());
    const auto &first_chord = get_const_chord(first_chord_number);
    auto first_note_number = top_left.row();
    if (get_level(bottom_right) == note_level) {
      auto bottom_right_chord_number = to_size_t(parent(bottom_right).row());
      if (first_chord_number == bottom_right_chord_number) {
        auto bottom_right_note_number = to_size_t(bottom_right.row());
        first_chord.copy_notes_to(
            first_note_number, bottom_right_note_number - first_note_number + 1,
            &note_chord_templates);
        return note_chord_templates;
      }
    }
    first_chord.copy_notes_to(first_note_number,
                              first_chord.notes.size() - first_note_number,
                              &note_chord_templates);
    copy_note_chord_templates_from_chord(first_chord_number + 1, bottom_right,
                                         &note_chord_templates);
  }
  return note_chord_templates;
}

void ChordsModel::replace_note_cells(
    size_t chord_number, size_t first_note_number,
    NoteChordField left_note_chord_field, NoteChordField right_note_chord_field,
    const std::vector<NoteChord> &note_chord_templates,
    size_t first_template_number, size_t write_number) {
  get_chord(chord_number)
      .replace_note_cells(first_note_number, left_note_chord_field,
                          right_note_chord_field, note_chord_templates,
                          first_template_number, write_number);
  emit dataChanged(
      get_note_index(chord_number, first_note_number, left_note_chord_field),
      get_note_index(chord_number, first_note_number + write_number - 1,
                     right_note_chord_field),
      {Qt::DisplayRole, Qt::EditRole});
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

void ChordsModel::set_chord_cell(size_t chord_number,
                                 NoteChordField note_chord_field,
                                 const QVariant &new_value) {
  get_chord(chord_number).setData(note_chord_field, new_value);
  auto index = get_chord_index(chord_number, note_chord_field);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, Qt::EditRole});
}

void ChordsModel::set_note_cell(size_t chord_number, size_t note_number,
                                NoteChordField note_chord_field,
                                const QVariant &new_value) {
  get_chord(chord_number)
      .get_note(note_number)
      .setData(note_chord_field, new_value);
  auto index = get_note_index(chord_number, note_number, note_chord_field);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::replace_chords_cells(
    size_t first_chord_number, NoteChordField left_note_chord_field,
    NoteChordField right_note_chord_field,
    const std::vector<NoteChord> &note_chord_templates,
    size_t first_template_number) {
  auto current_chord_number = first_chord_number;
  auto current_template_number = first_template_number;
  auto number_of_templates = note_chord_templates.size();
  while (current_template_number < number_of_templates) {
    auto &chord = get_chord(current_chord_number);
    chord.replace_cells(left_note_chord_field, right_note_chord_field,
                        note_chord_templates[current_template_number]);
    emit dataChanged(
        get_chord_index(current_chord_number, left_note_chord_field),
        get_chord_index(current_chord_number, right_note_chord_field),
        {Qt::DisplayRole, Qt::EditRole});
    current_template_number = current_template_number + 1;
    if (current_template_number >= number_of_templates) {
      break;
    }
    auto write_number = std::min(
        {number_of_templates - current_template_number, chord.notes.size()});
    replace_note_cells(current_chord_number, 0, left_note_chord_field,
                       right_note_chord_field, note_chord_templates,
                       current_template_number, write_number);
    current_template_number = current_template_number + write_number;
    current_chord_number = current_chord_number + 1;
  }
}

void ChordsModel::replace_notes_cells(
    size_t first_chord_number, size_t first_note_number,
    NoteChordField left_note_chord_field, NoteChordField right_note_chord_field,
    const std::vector<NoteChord> &note_chord_templates) {
  auto notes_size = get_chord(first_chord_number).notes.size();
  auto write_number =
      std::min({note_chord_templates.size(), notes_size - first_note_number});
  replace_note_cells(first_chord_number, first_note_number,
                     left_note_chord_field, right_note_chord_field,
                     note_chord_templates, 0, write_number);
  replace_chords_cells(first_chord_number + 1, left_note_chord_field,
                       right_note_chord_field, note_chord_templates,
                       write_number);
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
    if (!parse_clipboard(CHORDS_MIME)) {
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

void ChordsModel::paste_cells_or_after(const QItemSelection &selection) {
  auto top_left = top_left_index(selection);
  auto top_left_row = to_size_t(top_left.row());
  auto top_left_parent = parent(top_left);
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
                 {"required",
                  {"left_note_chord_field", "right_note_chord_field",
                   "note_chord_templates"}},
                 {"properties",
                  {{"left_note_chord_field",
                    {{"type", "number"},
                     {"description", "left NoteChord field"},
                     {"minimum", type_column},
                     {"maximum", words_column}}},
                   {"right_note_chord_field",
                    {{"type", "number"},
                     {"description", "right NoteChord field"},
                     {"minimum", type_column},
                     {"maximum", words_column}}},
                   {"note_chord_templates",
                    {{"type", "array"},
                     {"description", "a list of NoteChord templates"},
                     {"items",
                      {{"type", "object"},
                       {"description", "NoteChord templates"},
                       {"properties", get_note_chord_fields_schema()}}}}}}}}));
    if (!(validate_json(parent_pointer, clipboard, templates_validator))) {
      return;
    }

    Q_ASSERT(clipboard.contains("left_note_chord_field"));
    auto left_note_chord_field_value = clipboard["left_note_chord_field"];
    Q_ASSERT(left_note_chord_field_value.is_number());
    auto left_note_chord_field =
        to_note_chord_field(left_note_chord_field_value.get<int>());

    auto left_paste_column = to_note_chord_field(top_left.column());

    if (left_note_chord_field != left_paste_column) {
      QString message;
      QTextStream stream(&message);
      stream << tr("Destination left column ") << left_paste_column
             << tr(" doesn't match pasted left column ")
             << left_note_chord_field;
      QMessageBox::warning(parent_pointer, tr("Column number error"), message);
      return;
    }
    Q_ASSERT(clipboard.contains("right_note_chord_field"));
    auto right_note_chord_field_value = clipboard["right_note_chord_field"];
    Q_ASSERT(right_note_chord_field_value.is_number());

    Q_ASSERT(clipboard.contains("note_chord_templates"));
    auto &json_note_chord_templates = clipboard["note_chord_templates"];

    size_t number_of_additional_note_chords = 0;
    auto number_of_templates = json_note_chord_templates.size();
    if (get_level(top_left) == chord_level) {
      number_of_additional_note_chords =
          get_number_of_additional_note_chords(top_left_row);

    } else {
      auto chord_number = to_size_t(top_left_parent.row());
      number_of_additional_note_chords =
          get_chord(chord_number).notes.size() - top_left_row +
          get_number_of_additional_note_chords(chord_number + 1);
    }

    if (number_of_templates > number_of_additional_note_chords) {
      QString message;
      QTextStream stream(&message);
      stream << tr("Pasted ") << number_of_templates << tr(" rows but only ")
             << number_of_additional_note_chords << tr(" rows available");
      QMessageBox::warning(parent_pointer, tr("Row number error"), message);
      return;
    }

    std::vector<NoteChord> note_chord_templates;
    std::transform(json_note_chord_templates.cbegin(),
                   json_note_chord_templates.cend(),
                   std::back_inserter(note_chord_templates),
                   [](const nlohmann::json &json_template) {
                     return NoteChord(json_template);
                   });

    auto bottom_right = get_bottom_right_index(
        top_left, to_note_chord_field(right_note_chord_field_value.get<int>()),
        note_chord_templates.size() - 1);
    add_cell_changes(top_left, bottom_right,
                     copy_note_chord_templates(top_left, bottom_right),
                     note_chord_templates);
    return;
  }
}

void ChordsModel::delete_selected(const QItemSelection &selection) {
  auto top_left = top_left_index(selection);
  auto bottom_right = bottom_right_index(selection);
  if (is_rows(selection)) {
    auto top_left_row = top_left.row();
    removeRows(top_left_row, bottom_right.row() - top_left_row + 1,
               top_left.parent());
  } else {
    auto old_note_chord_templates =
        copy_note_chord_templates(top_left, bottom_right);
    add_cell_changes(top_left, bottom_right, old_note_chord_templates,
                     std::vector<NoteChord>(old_note_chord_templates.size()));
  }
}

void ChordsModel::copy_selected(const QItemSelection &selection) const {
  auto top_left = top_left_index(selection);
  auto bottom_right = bottom_right_index(selection);
  if (is_rows(selection)) {
    auto number_of_children = bottom_right.row() - top_left.row() + 1;
    if (get_level(top_left) == chord_level) {
      copy_json(
          copy_chords_to_json(to_size_t(top_left.row()), number_of_children),
          CHORDS_MIME);
    } else {
      copy_json(get_const_chord(to_size_t(parent(top_left).row()))
                    .copy_notes_to_json(to_size_t(top_left.row()),
                                        number_of_children),
                NOTES_MIME);
    }
  } else {
    const auto note_chord_templates =
        copy_note_chord_templates(top_left, bottom_right);
    nlohmann::json json_note_chords;
    std::transform(
        note_chord_templates.cbegin(), note_chord_templates.cend(),
        std::back_inserter(json_note_chords),
        [](const NoteChord &note_chord) { return note_chord.json(); });
    copy_json(nlohmann::json({{"left_note_chord_field", top_left.column()},
                              {"right_note_chord_field", bottom_right.column()},
                              {"note_chord_templates", json_note_chords}}),
              TEMPLATES_MIME);
  }
}