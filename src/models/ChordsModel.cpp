#include "justly/ChordsModel.hpp"

#include <QAbstractItemModel>
#include <QBrush>
#include <QByteArray>
#include <QClipboard>
#include <QGuiApplication>
#include <QItemSelectionModel>
#include <QList>
#include <QMessageBox>
#include <QMimeData>
#include <QObject>
#include <QPalette>
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
#include <numeric>
#include <sstream>
#include <string>
#include <utility>
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
#include "justly/NoteChordColumn.hpp"
#include "justly/Rational.hpp"
#include "justly/RowRange.hpp"
#include "other/conversions.hpp"

const auto CHORDS_MIME = "application/prs.chords+json";
const auto NOTES_MIME = "application/prs.notes+json";
const auto CELLS_MIME = "application/prs.cells+json";

[[nodiscard]] auto get_note_chord_column(const QModelIndex &index) {
  return to_note_chord_column(index.column());
}

[[nodiscard]] auto
get_column_name(NoteChordColumn note_chord_column) -> QString {
  switch (note_chord_column) {
  case type_column:
    return ChordsModel::tr("Type");
  case instrument_column:
    return ChordsModel::tr("Instrument");
  case interval_column:
    return ChordsModel::tr("Interval");
  case beats_column:
    return ChordsModel::tr("Beats");
  case volume_ratio_column:
    return ChordsModel::tr("Volume ratio");
  case tempo_ratio_column:
    return ChordsModel::tr("Tempo ratio");
  case words_column:
    return ChordsModel::tr("Words");
  default:
    Q_ASSERT(false);
    return {};
  }
}

[[nodiscard]] auto is_rows(const QItemSelection &selection) {
  // selecting a type column selects the whole row
  return selection[0].left() == type_column;
}

[[nodiscard]] auto get_mime_description(const QString &mime_type) -> QString {
  if (mime_type == CHORDS_MIME) {
    return ChordsModel::tr("chords");
  }
  if (mime_type == NOTES_MIME) {
    return ChordsModel::tr("notes");
  }
  if (mime_type == CELLS_MIME) {
    return ChordsModel::tr("cells");
  }
  return mime_type;
}

[[nodiscard]] auto
get_note_chord_column_schema(const std::string &description) {
  return nlohmann::json({{"type", "number"},
                         {"description", description},
                         {"minimum", type_column},
                         {"maximum", words_column}});
}

[[nodiscard]] auto copy_json(const nlohmann::json &copied,
                             const QString &mime_type) {
  std::stringstream json_text;
  json_text << std::setw(4) << copied;
  copy_text(json_text.str(), mime_type);
}

void note_chords_from_json(std::vector<NoteChord> *new_note_chords_pointer,
                           const nlohmann::json &json_note_chords,
                           size_t number_to_write) {
  Q_ASSERT(new_note_chords_pointer != nullptr);
  std::transform(json_note_chords.cbegin(),
                 json_note_chords.cbegin() + static_cast<int>(number_to_write),
                 std::back_inserter(*new_note_chords_pointer),
                 [](const nlohmann::json &json_template) {
                   return NoteChord(json_template);
                 });
}

auto get_child_number(const QModelIndex &index) -> size_t {
  return to_size_t(index.row());
}

auto is_root_index(const QModelIndex &index) -> bool {
  // root index is invalid
  return !index.isValid();
}

auto valid_is_chord_index(const QModelIndex &index) -> bool {
  Q_ASSERT(!is_root_index(index));
  // chords have null parent pointers
  return index.internalPointer() == nullptr;
}

auto make_validator(const std::string &title, nlohmann::json json)
    -> nlohmann::json_schema::json_validator {
  json["$schema"] = "http://json-schema.org/draft-07/schema#";
  json["title"] = title;
  return {json};
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
  Q_ASSERT(chord_number < get_number_of_chords());
}

void ChordsModel::check_chord_number_end(size_t chord_number) const {
  Q_ASSERT(chord_number <= get_number_of_chords());
}

void ChordsModel::check_chord_range(size_t first_chord_number,
                                    size_t number_of_chords) const {
  check_chord_number(first_chord_number);
  check_chord_number_end(first_chord_number + number_of_chords);
}

auto ChordsModel::get_const_chord(size_t chord_number) const -> const Chord & {
  check_chord_number(chord_number);
  return chords[chord_number];
}

auto ChordsModel::get_number_of_chords() const -> size_t {
  return chords.size();
};

auto ChordsModel::get_chord(size_t chord_number) -> Chord & {
  check_chord_number(chord_number);
  return chords[chord_number];
}

auto ChordsModel::get_number_of_rows_left(size_t first_chord_number) const
    -> size_t {
  return std::accumulate(chords.begin() + static_cast<int>(first_chord_number),
                         chords.end(), static_cast<size_t>(0),
                         [](int total, const Chord &chord) {
                           // add 1 for the chord itself
                           return total + 1 + chord.get_number_of_notes();
                         });
}

[[nodiscard]] auto ChordsModel::get_note_chords_from_ranges(
    const std::vector<RowRange> &row_ranges) const -> std::vector<NoteChord> {
  std::vector<NoteChord> note_chords;
  for (const auto &row_range : row_ranges) {
    auto first_child_number = row_range.first_child_number;
    auto number_of_children = row_range.number_of_children;
    if (row_range.is_chords()) {
      check_chord_range(first_child_number, number_of_children);
      note_chords.insert(
          note_chords.end(),
          chords.cbegin() + static_cast<int>(first_child_number),
          chords.cbegin() + static_cast<int>(row_range.get_end_child_number()));

    } else {
      chords[row_range.get_parent_chord_number()].copy_notes_to_notechords(
          first_child_number, number_of_children, &note_chords);
    }
  }
  return note_chords;
}

auto ChordsModel::parse_clipboard(
    const QString &mime_type,
    const nlohmann::json_schema::json_validator &validator) -> nlohmann::json {
  const auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  const auto *mime_data_pointer = clipboard_pointer->mimeData();
  Q_ASSERT(mime_data_pointer != nullptr);

  if (!mime_data_pointer->hasFormat(mime_type)) {
    auto formats = mime_data_pointer->formats();
    Q_ASSERT(!(formats.empty()));
    QString message;
    QTextStream stream(&message);
    stream << tr("Cannot paste ") << get_mime_description(formats[0])
           << tr(" into destination needing ")
           << get_mime_description(mime_type);
    QMessageBox::warning(parent_pointer, tr("MIME type error"), message);
    return {};
  }
  const auto &copied_text = mime_data_pointer->data(mime_type).toStdString();
  nlohmann::json copied;
  try {
    copied = nlohmann::json::parse(copied_text);
  } catch (const nlohmann::json::parse_error &parse_error) {
    QMessageBox::warning(parent_pointer, tr("Parsing error"),
                         parse_error.what());
    return {};
  }
  if (copied.empty()) {
    QMessageBox::warning(parent_pointer, tr("Empty paste"),
                         "Nothing to paste!");
    return {};
  }
  try {
    validator.validate(copied);
  } catch (const std::exception &error) {
    QMessageBox::warning(parent_pointer, tr("Schema error"), error.what());
    return {};
  }
  return copied;
}

void ChordsModel::add_cell_change(const QModelIndex &index,
                                  const QVariant &new_value) {
  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->push(
      std::make_unique<CellChange>(this,
                                   CellIndex(get_child_number(index),
                                             get_note_chord_column(index),
                                             index.parent().row()),
                                   data(index, Qt::EditRole), new_value)
          .release());
}

void ChordsModel::add_cell_changes(
    const std::vector<RowRange> &row_ranges, NoteChordColumn left_field,
    NoteChordColumn right_field,
    const std::vector<NoteChord> &new_note_chords) {
  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->push(
      std::make_unique<CellChanges>(this, row_ranges, left_field, right_field,
                                    get_note_chords_from_ranges(row_ranges),
                                    new_note_chords)
          .release());
}

void ChordsModel::add_row_ranges_from(std::vector<RowRange> *row_range_pointer,
                                      size_t chord_number,
                                      size_t number_of_note_chords) const {
  while (number_of_note_chords > 0) {
    row_range_pointer->emplace_back(chord_number, 1, -1);
    number_of_note_chords = number_of_note_chords - 1;
    if (number_of_note_chords == 0) {
      break;
    }
    auto number_of_notes = get_const_chord(chord_number).get_number_of_notes();
    if (number_of_notes > 0) {
      if (number_of_note_chords <= number_of_notes) {
        row_range_pointer->emplace_back(0, number_of_note_chords, chord_number);
        break;
      }
      row_range_pointer->emplace_back(0, number_of_notes, chord_number);
      number_of_note_chords = number_of_note_chords - number_of_notes;
    }
    chord_number = chord_number + 1;
  }
}

ChordsModel::ChordsModel(QUndoStack *undo_stack_pointer_input,
                         QWidget *parent_pointer_input)
    : QAbstractItemModel(parent_pointer_input),
      parent_pointer(parent_pointer_input),
      undo_stack_pointer(undo_stack_pointer_input) {}

auto ChordsModel::get_chord_index(size_t chord_number,
                                  NoteChordColumn note_chord_column) const
    -> QModelIndex {
  return createIndex(static_cast<int>(chord_number), note_chord_column,
                     nullptr);
}

auto ChordsModel::get_note_index(size_t chord_number, size_t note_number,
                                 NoteChordColumn note_chord_column) const
    -> QModelIndex {
  return createIndex(static_cast<int>(note_number), note_chord_column,
                     &get_const_chord(chord_number));
}

auto ChordsModel::rowCount(const QModelIndex &parent_index) const -> int {
  if (is_root_index(parent_index)) {
    return static_cast<int>(get_number_of_chords());
  }
  // only nest into the symbol cell
  if (valid_is_chord_index(parent_index) &&
      get_note_chord_column(parent_index) == type_column) {
    return static_cast<int>(
        get_const_chord(get_child_number(parent_index)).get_number_of_notes());
  }
  // notes and non-symbol chord cells have no children
  return 0;
}

auto ChordsModel::columnCount(const QModelIndex & /*parent*/) const -> int {
  return NUMBER_OF_NOTE_CHORD_COLUMNS;
}

// get the parent index
auto ChordsModel::parent(const QModelIndex &index) const -> QModelIndex {
  // chords have null parents
  if (valid_is_chord_index(index)) {
    return {};
  }
  // notes are nested into the type column
  // the internal pointer is the pointer to the parent chord
  // use std::distance to get the chord number from the pointer
  return createIndex(
      static_cast<int>(std::distance(
          chords.data(), static_cast<const Chord *>(index.internalPointer()))),
      type_column, nullptr);
}

auto ChordsModel::index(int signed_first_child_number, int column,
                        const QModelIndex &parent_index) const -> QModelIndex {
  auto child_number = to_size_t(signed_first_child_number);
  auto note_chord_column = to_note_chord_column(column);
  if (is_root_index(parent_index)) {
    return get_chord_index(child_number, note_chord_column);
  }
  return get_note_index(get_child_number(parent_index), child_number,
                        note_chord_column);
}

auto ChordsModel::headerData(int column, Qt::Orientation orientation,
                             int role) const -> QVariant {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    return get_column_name(to_note_chord_column(column));
  }
  // no horizontal headers
  // no headers for other roles
  return {};
}

auto ChordsModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  auto selectable = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  // type column isn't editable
  return get_note_chord_column(index) == type_column
             ? selectable
             : (selectable | Qt::ItemIsEditable);
}

auto ChordsModel::data(const QModelIndex &index, int role) const -> QVariant {
  const NoteChord *note_chord_pointer = nullptr;
  auto child_number = get_child_number(index);
  if (valid_is_chord_index(index)) {
    note_chord_pointer = &get_const_chord(child_number);
  } else {
    note_chord_pointer = &get_const_chord(get_child_number(index.parent()))
                              .get_const_note(child_number);
  }
  Q_ASSERT(note_chord_pointer != nullptr);
  if (role == Qt::BackgroundRole) {
    const auto &palette = parent_pointer->palette();
    return note_chord_pointer->is_chord() ? palette.base()
                                          : palette.alternateBase();
  }
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    return note_chord_pointer->data(get_note_chord_column(index));
  }
  return {};
}

auto ChordsModel::setData(const QModelIndex &index, const QVariant &new_value,
                          int role) -> bool {
  // only set data for edit
  if (role != Qt::EditRole) {
    return false;
  }
  add_cell_change(index, new_value);
  return true;
}

auto ChordsModel::insertRows(int signed_first_child_number,
                             int signed_number_of_children,
                             const QModelIndex &parent_index) -> bool {
  Q_ASSERT(undo_stack_pointer != nullptr);
  auto first_child_number = to_size_t(signed_first_child_number);
  auto number_of_children = to_size_t(signed_number_of_children);

  if (is_root_index(parent_index)) {
    Chord template_chord;
    if (first_child_number > 0) {
      template_chord.beats = get_const_chord(first_child_number - 1).beats;
    }

    std::vector<Chord> new_chords;
    for (size_t index = 0; index < number_of_children; index = index + 1) {
      new_chords.push_back(template_chord);
    }
    undo_stack_pointer->push(
        std::make_unique<InsertChords>(this, first_child_number, new_chords)
            .release());
  } else {
    auto chord_number = get_child_number(parent_index);
    const auto &parent_chord = get_const_chord(chord_number);

    Note template_note;

    if (first_child_number == 0) {
      template_note.beats = parent_chord.beats;
      template_note.words = parent_chord.words;
    } else {
      const auto &previous_note =
          parent_chord.get_const_note(first_child_number - 1);

      template_note.beats = previous_note.beats;
      template_note.volume_ratio = previous_note.volume_ratio;
      template_note.tempo_ratio = previous_note.tempo_ratio;
      template_note.words = previous_note.words;
    }

    std::vector<Note> new_notes;
    for (size_t index = 0; index < number_of_children; index = index + 1) {
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

auto ChordsModel::removeRows(int signed_first_child_number,
                             int signed_number_of_children,
                             const QModelIndex &parent_index) -> bool {
  auto first_child_number = to_size_t(signed_first_child_number);
  auto number_of_children = to_size_t(signed_number_of_children);

  Q_ASSERT(undo_stack_pointer != nullptr);
  if (is_root_index(parent_index)) {
    check_chord_range(first_child_number, number_of_children);
    undo_stack_pointer->push(
        std::make_unique<RemoveChords>(
            this, first_child_number,
            std::vector<Chord>(
                chords.cbegin() + signed_first_child_number,
                chords.cbegin() +
                    static_cast<int>(first_child_number + number_of_children)))
            .release());
  } else {
    auto chord_number = get_child_number(parent_index);
    const auto &chord = get_const_chord(chord_number);
    undo_stack_pointer->push(
        std::make_unique<RemoveNotes>(
            this, chord_number, first_child_number,
            chord.copy_notes(first_child_number, number_of_children))
            .release());
  }
  return true;
}

void ChordsModel::set_cell(const CellIndex &cell_index,
                           const QVariant &new_value) {
  auto child_number = cell_index.child_number;
  auto note_chord_column = cell_index.note_chord_column;

  QModelIndex index;
  if (cell_index.is_chords()) {
    get_chord(child_number).setData(note_chord_column, new_value);
    index = get_chord_index(child_number, note_chord_column);
  } else {
    auto chord_number = cell_index.get_parent_chord_number();
    get_chord(chord_number)
        .set_note_data(child_number, note_chord_column, new_value);
    index = get_note_index(chord_number, child_number, note_chord_column);
  }
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::replace_cell_ranges(
    const std::vector<RowRange> &row_ranges, NoteChordColumn left_field,
    NoteChordColumn right_field, const std::vector<NoteChord> &note_chords) {
  size_t note_chord_number = 0;
  for (const auto &row_range : row_ranges) {
    auto first_child_number = row_range.first_child_number;
    auto number_of_children = row_range.number_of_children;
    auto last_child_number = row_range.get_end_child_number() - 1;
    QModelIndex first_index;
    QModelIndex last_index;
    if (row_range.is_chords()) {
      for (size_t write_number = 0; write_number < number_of_children;
           write_number++) {
        auto write_note_chord_number = note_chord_number + write_number;
        Q_ASSERT(write_note_chord_number < note_chords.size());
        get_chord(first_child_number + write_number)
            .replace_cells(left_field, right_field,
                           note_chords[write_note_chord_number]);
      }
      first_index = get_chord_index(first_child_number, left_field);
      last_index = get_chord_index(last_child_number, right_field);
    } else {
      auto chord_number = row_range.get_parent_chord_number();
      get_chord(chord_number)
          .replace_note_cells(first_child_number, number_of_children,
                              left_field, right_field, note_chords,
                              note_chord_number);
      first_index =
          get_note_index(chord_number, first_child_number, left_field);
      last_index = get_note_index(chord_number, last_child_number, right_field);
    }
    emit dataChanged(first_index, last_index, {Qt::DisplayRole, Qt::EditRole});
    note_chord_number = note_chord_number + number_of_children;
  }
}

auto ChordsModel::copy_chords_to_json(size_t first_chord_number,
                                      size_t number_of_chords) const
    -> nlohmann::json {
  nlohmann::json json_chords;

  check_chord_range(first_chord_number, number_of_chords);
  std::transform(chords.cbegin() + static_cast<int>(first_chord_number),
                 chords.cbegin() +
                     static_cast<int>(first_chord_number + number_of_chords),
                 std::back_inserter(json_chords),
                 [](const Chord &chord) { return chord.to_json(); });
  return json_chords;
}

auto ChordsModel::to_json() const -> nlohmann::json {
  return copy_chords_to_json(0, get_number_of_chords());
}

void ChordsModel::paste_rows(size_t first_child_number,
                             const QModelIndex &parent_index) {
  Q_ASSERT(undo_stack_pointer != nullptr);
  if (is_root_index(parent_index)) {
    static const nlohmann::json_schema::json_validator chords_validator =
        make_validator("Chords", get_chords_schema());
    auto json_chords = parse_clipboard(CHORDS_MIME, chords_validator);
    if (json_chords.empty()) {
      return;
    }
    std::vector<Chord> new_chords;
    std::transform(
        json_chords.cbegin(), json_chords.cend(),
        std::back_inserter(new_chords),
        [](const nlohmann::json &json_chord) { return Chord(json_chord); });

    undo_stack_pointer->push(
        std::make_unique<InsertChords>(this, first_child_number, new_chords)
            .release());
  } else {
    static const nlohmann::json_schema::json_validator notes_validator =
        make_validator("Notes", get_notes_schema());
    auto json_notes = parse_clipboard(NOTES_MIME, notes_validator);
    if (json_notes.empty()) {
      return;
    }
    std::vector<Note> new_notes;
    std::transform(
        json_notes.cbegin(), json_notes.cend(), std::back_inserter(new_notes),
        [](const nlohmann::json &json_note) { return Note(json_note); });
    undo_stack_pointer->push(
        std::make_unique<InsertNotes>(this, get_child_number(parent_index),
                                      first_child_number, new_notes)
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

void ChordsModel::append_json_chords(const nlohmann::json &json_chords) {
  auto chords_size = get_number_of_chords();
  beginInsertRows(QModelIndex(), static_cast<int>(chords_size),
                  static_cast<int>(chords_size + json_chords.size()) - 1);
  std::transform(
      json_chords.cbegin(), json_chords.cend(), std::back_inserter(chords),
      [](const nlohmann::json &json_chord) { return Chord(json_chord); });
  endInsertRows();
}

void ChordsModel::remove_chords(size_t first_chord_number,
                                size_t number_of_chords) {
  check_chord_range(first_chord_number, number_of_chords);

  auto int_first_chord_number = static_cast<int>(first_chord_number);
  auto int_end_child_number =
      static_cast<int>(first_chord_number + number_of_chords);

  beginRemoveRows(QModelIndex(), int_first_chord_number,
                  int_end_child_number - 1);
  chords.erase(chords.begin() + int_first_chord_number,
               chords.begin() + int_end_child_number);
  endRemoveRows();
}

void ChordsModel::delete_all_chords() {
  if (!chords.empty()) {
    remove_chords(0, get_number_of_chords());
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

void ChordsModel::remove_notes(size_t chord_number, size_t first_note_number,
                               size_t number_of_notes) {
  beginRemoveRows(get_chord_index(chord_number),
                  static_cast<int>(first_note_number),
                  static_cast<int>(first_note_number + number_of_notes - 1));
  get_chord(chord_number).remove_notes(first_note_number, number_of_notes);
  endRemoveRows();
}

void ChordsModel::copy_selected(const QItemSelection &selection) const {
  if (is_rows(selection)) {
    Q_ASSERT(selection.size() == 1);
    const auto &row_range = RowRange(selection[0]);
    auto first_child_number = row_range.first_child_number;
    auto number_of_children = row_range.number_of_children;
    if (row_range.is_chords()) {
      copy_json(copy_chords_to_json(first_child_number, number_of_children),
                CHORDS_MIME);
    } else {
      copy_json(get_const_chord(row_range.get_parent_chord_number())
                    .copy_notes_to_json(first_child_number, number_of_children),
                NOTES_MIME);
    }
  } else {
    Q_ASSERT(!selection.empty());
    const auto &first_range = selection[0];
    const auto row_ranges = to_row_ranges(selection);
    const auto note_chords = get_note_chords_from_ranges(row_ranges);
    nlohmann::json json_note_chords;
    std::transform(
        note_chords.cbegin(), note_chords.cend(),
        std::back_inserter(json_note_chords),
        [](const NoteChord &note_chord) { return note_chord.to_json(); });
    copy_json(nlohmann::json(
                  {{"left_field", to_note_chord_column(first_range.left())},
                   {"right_field", to_note_chord_column(first_range.right())},
                   {"note_chords", std::move(json_note_chords)}}),
              CELLS_MIME);
  }
}

void ChordsModel::paste_cells_or_after(const QItemSelection &selection) {
  Q_ASSERT(!selection.empty());
  if (is_rows(selection)) {
    Q_ASSERT(selection.size() == 1);
    const auto &range = selection[0];
    paste_rows(range.bottom() + 1, range.parent());
  } else {
    auto first_selected_row_range = get_first_row_range(selection);
    auto number_of_selected_rows = get_number_of_rows(selection);
    auto first_selected_child_number =
        first_selected_row_range.first_child_number;
    auto first_selected_is_chords = first_selected_row_range.is_chords();

    static const nlohmann::json_schema::json_validator cells_validator =
        make_validator(
            "Cells",
            nlohmann::json(
                {{"description", "cells"},
                 {"type", "object"},
                 {"required", {"left_field", "right_field", "note_chords"}},
                 {"properties",
                  {{"left_field",
                    get_note_chord_column_schema("left NoteChordColumn")},
                   {"right_field",
                    get_note_chord_column_schema("right NoteChordColumn")},
                   {"note_chords",
                    {{"type", "array"},
                     {"description", "a list of NoteChords"},
                     {"items",
                      {{"type", "object"},
                       {"description", "a NoteChord"},
                       {"properties", get_note_chord_columns_schema()}}}}}}}}));
    auto json_cells = parse_clipboard(CELLS_MIME, cells_validator);
    if (json_cells.empty()) {
      return;
    }
    Q_ASSERT(json_cells.contains("left_field"));
    auto left_field_value = json_cells["left_field"];
    Q_ASSERT(left_field_value.is_number());

    Q_ASSERT(json_cells.contains("right_field"));
    auto right_field_value = json_cells["right_field"];
    Q_ASSERT(right_field_value.is_number());

    Q_ASSERT(json_cells.contains("note_chords"));
    auto &json_note_chords = json_cells["note_chords"];

    size_t number_of_rows_left = 0;
    auto number_of_note_chords = json_note_chords.size();
    if (first_selected_is_chords) {
      number_of_rows_left =
          get_number_of_rows_left(first_selected_child_number);
    } else {
      auto first_selected_chord_number =
          first_selected_row_range.get_parent_chord_number();
      number_of_rows_left =
          get_chord(first_selected_chord_number).get_number_of_notes() -
          first_selected_child_number +
          get_number_of_rows_left(first_selected_chord_number + 1);
    }

    std::vector<NoteChord> new_note_chords;
    if (number_of_selected_rows > number_of_note_chords) {
      size_t written = 0;
      while (number_of_selected_rows - written > number_of_note_chords) {
        note_chords_from_json(&new_note_chords, json_note_chords,
                              number_of_note_chords);
        written = written + number_of_note_chords;
      }
      note_chords_from_json(&new_note_chords, json_note_chords,
                            number_of_selected_rows - written);
    } else {
      note_chords_from_json(&new_note_chords, json_note_chords,
                            number_of_note_chords > number_of_rows_left
                                ? number_of_rows_left
                                : number_of_note_chords);
    }
    auto number_of_copied_note_chords = new_note_chords.size();
    std::vector<RowRange> row_ranges;
    if (first_selected_is_chords) {
      add_row_ranges_from(&row_ranges, first_selected_child_number,
                          number_of_copied_note_chords);
    } else {
      auto chord_number = first_selected_row_range.get_parent_chord_number();
      auto number_of_notes_left =
          get_const_chord(chord_number).get_number_of_notes() -
          first_selected_child_number;
      if (number_of_copied_note_chords <= number_of_notes_left) {
        row_ranges.emplace_back(first_selected_child_number,
                                number_of_copied_note_chords, chord_number);
      } else {
        row_ranges.emplace_back(first_selected_child_number,
                                number_of_notes_left, chord_number);
        add_row_ranges_from(&row_ranges, chord_number + 1,
                            number_of_copied_note_chords -
                                number_of_notes_left);
      }
    }
    add_cell_changes(
        row_ranges, to_note_chord_column(left_field_value.get<int>()),
        to_note_chord_column(right_field_value.get<int>()), new_note_chords);
  }
}

void ChordsModel::delete_selected(const QItemSelection &selection) {
  if (is_rows(selection)) {
    Q_ASSERT(selection.size() == 1);
    const auto &range = selection[0];
    const auto &row_range = RowRange(selection[0]);
    auto removed = removeRows(static_cast<int>(row_range.first_child_number),
                              static_cast<int>(row_range.number_of_children),
                              range.parent());
    Q_ASSERT(removed);
  } else {
    Q_ASSERT(!selection.empty());
    const auto &first_range = selection[0];
    add_cell_changes(to_row_ranges(selection),
                     to_note_chord_column(first_range.left()),
                     to_note_chord_column(first_range.right()),
                     std::vector<NoteChord>(get_number_of_rows(selection)));
  }
}
