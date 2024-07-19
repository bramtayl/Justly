#include "justly/ChordsModel.hpp"

#include <QAbstractItemModel>
#include <QByteArray>
#include <QClipboard>
#include <QGuiApplication>
#include <QList>
#include <QMessageBox>
#include <QMetaType>
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
#include <iterator>
// IWYU pragma: no_include <map>
#include <memory>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "changes/ChordCellChange.hpp"
#include "changes/InsertChords.hpp"
#include "changes/InsertJsonChords.hpp"
#include "changes/InsertJsonNotes.hpp"
#include "changes/InsertNotes.hpp"
#include "changes/NoteCellChange.hpp"
#include "changes/RemoveChords.hpp"
#include "changes/RemoveNotes.hpp"
#include "justly/Chord.hpp"
#include "justly/Instrument.hpp"
#include "justly/Interval.hpp"
#include "justly/Note.hpp"
#include "justly/NoteChord.hpp"
#include "justly/NoteChordField.hpp"
#include "justly/Rational.hpp"
#include "justly/SelectionType.hpp"
#include "justly/TreeLevel.hpp"
#include "justly/public_constants.hpp"
#include "other/private.hpp"

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

auto ChordsModel::parse_clipboard(SelectionType selection_type) -> bool {
  const auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  const auto *mime_data_pointer = clipboard_pointer->mimeData();
  Q_ASSERT(mime_data_pointer != nullptr);

  auto mime_type = get_mime_type(selection_type);
  if (!mime_data_pointer->hasFormat(mime_type)) {
    auto formats = mime_data_pointer->formats();
    Q_ASSERT(!(formats.empty()));
    QString message;
    QTextStream stream(&message);
    stream << tr("Cannot paste MIME type \"") << formats[0]
           << tr("\" into destination needing MIME type \"") << mime_type;
    QMessageBox::warning(parent_pointer, tr("MIME type error"), message);
    return {};
  }
  const auto &copied_text = mime_data_pointer->data(mime_type).toStdString();
  try {
    clipboard = nlohmann::json::parse(copied_text);
  } catch (const nlohmann::json::parse_error &parse_error) {
    QMessageBox::warning(parent_pointer, tr("Parsing error"),
                         parse_error.what());
    return false;
  }
  static const nlohmann::json_schema::json_validator rational_validator = []() {
    auto rational_schema = get_rational_schema("rational");
    rational_schema["$schema"] = "http://json-schema.org/draft-07/schema#";
    rational_schema["title"] = "Rational";
    return nlohmann::json_schema::json_validator(rational_schema);
  }();
  static const nlohmann::json_schema::json_validator interval_validator = []() {
    auto interval_schema = get_interval_schema();
    interval_schema["$schema"] = "http://json-schema.org/draft-07/schema#";
    interval_schema["title"] = "Interval";
    return interval_schema;
  }();
  static const nlohmann::json_schema::json_validator instrument_validator =
      []() {
        auto instrument_schema = get_instrument_schema();
        instrument_schema["$schema"] =
            "http://json-schema.org/draft-07/schema#";
        instrument_schema["title"] = "Instrument";
        return nlohmann::json_schema::json_validator(instrument_schema);
      }();
  static const nlohmann::json_schema::json_validator words_validator = []() {
    auto words_schema = get_words_schema();
    words_schema["$schema"] = "http://json-schema.org/draft-07/schema#";
    words_schema["title"] = "Words";
    return nlohmann::json_schema::json_validator(words_schema);
  }();
  static const nlohmann::json_schema::json_validator notes_validator = []() {
    auto notes_schema = get_notes_schema();
    notes_schema["$schema"] = "http://json-schema.org/draft-07/schema#";
    notes_schema["title"] = "Notes";
    return nlohmann::json_schema::json_validator(notes_schema);
  }();
  static const nlohmann::json_schema::json_validator chords_validator(
      nlohmann::json({
          {"$schema", "http://json-schema.org/draft-07/schema#"},
          {"title", "Chords"},
          {"description", "a list of chords"},
          {"type", "array"},
          {"items", get_chord_schema()},
      }));

  switch (selection_type) {
  case instrument_type:
    return validate_json(parent_pointer, clipboard, instrument_validator);
  case interval_type:
    return validate_json(parent_pointer, clipboard, interval_validator);
  case rational_type:
    return validate_json(parent_pointer, clipboard, rational_validator);
  case words_type:
    return validate_json(parent_pointer, clipboard, words_validator);
  case notes_type:
    return validate_json(parent_pointer, clipboard, notes_validator);
  case chords_type:
    return validate_json(parent_pointer, clipboard, chords_validator);
  default:
    Q_ASSERT(false);
    return false;
  }
}

auto get_level(QModelIndex index) -> TreeLevel {
  // root will be an invalid index
  return !index.isValid() ? root_level
         // chords have null parent pointers
         : index.internalPointer() == nullptr ? chord_level
                                              : note_level;
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

void ChordsModel::add_cell_change(const QModelIndex &index,
                                  const QVariant &new_value) {
  Q_ASSERT(undo_stack_pointer != nullptr);

  auto child_number = to_size_t(index.row());
  auto note_chord_field = to_note_chord_field(index.column());

  if (get_level(index) == chord_level) {
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
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, Qt::EditRole});
}

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

void ChordsModel::copy_cell(const QModelIndex &index) const {
  const auto *note_chord_pointer = get_const_note_chord_pointer(index);
  Q_ASSERT(note_chord_pointer != nullptr);
  note_chord_pointer->copy_cell(to_note_chord_field(index.column()));
}

void ChordsModel::paste_cell(const QModelIndex &index) {
  auto selection_type = get_selection_type(to_note_chord_field(index.column()));
  if (parse_clipboard(selection_type)) {
    switch (selection_type) {
    case rational_type:
      add_cell_change(index, QVariant::fromValue(Rational(clipboard)));
      break;
    case interval_type:
      add_cell_change(index, QVariant::fromValue(Interval(clipboard)));
      break;
    case instrument_type:
      add_cell_change(index, QVariant::fromValue(get_instrument_pointer(
                                 clipboard.get<std::string>())));
      break;
    case words_type:
      add_cell_change(index,
                      QString::fromStdString(clipboard.get<std::string>()));
      break;
    default:
      Q_ASSERT(false);
      return;
    }
  }
}

void ChordsModel::copy_rows(size_t first_child_number,
                            size_t number_of_children,
                            const QModelIndex &parent_index) const {
  if (get_level(parent_index) == root_level) {
    copy_json(copy_chords_to_json(first_child_number, number_of_children),
              CHORDS_MIME);
  } else {
    copy_json(get_const_chord(to_size_t(parent_index.row()))
                  .copy_notes_to_json(first_child_number, number_of_children),
              NOTES_MIME);
  }
}

void ChordsModel::paste_rows(size_t first_child_number,
                             const QModelIndex &parent_index) {
  auto selection_type =
      get_level(parent_index) == root_level ? chords_type : notes_type;
  if (parse_clipboard(selection_type)) {
    Q_ASSERT(undo_stack_pointer != nullptr);
    if (selection_type == chords_type) {
      undo_stack_pointer->push(std::make_unique<InsertJsonChords>(
                                   this, first_child_number, clipboard)
                                   .release());
    } else {
      undo_stack_pointer->push(
          std::make_unique<InsertJsonNotes>(this, to_size_t(parent_index.row()),
                                            first_child_number, clipboard)
              .release());
    }
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
