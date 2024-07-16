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
#include "justly/TreeLevel.hpp"
#include "justly/public_constants.hpp"
#include "other/private.hpp"

auto get_column_name(NoteChordField note_chord_field) {
  switch (note_chord_field) {
  case interval_column:
    return "Interval";
  case beats_column:
    return "Beats";
  case volume_ratio_column:
    return "Volume ratio";
  case tempo_ratio_column:
    return "Tempo ratio";
  case words_column:
    return "Words";
  case instrument_column:
    return "Instrument";
  default:
    Q_ASSERT(false);
    return "";
  }
}

auto get_mime_data_pointer() {
  const auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  return clipboard_pointer->mimeData();
}

auto copy_json(const nlohmann::json &copied, const char *mime_type) {
  std::stringstream json_text;
  json_text << std::setw(4) << copied;
  copy_text(json_text.str(), mime_type);
}

auto get_copied_text(const QMimeData *mime_data_pointer,
                     const char *mime_type) {
  Q_ASSERT(mime_data_pointer != nullptr);
  Q_ASSERT(mime_data_pointer->hasFormat(mime_type));
  return mime_data_pointer->data(mime_type).toStdString();
}

auto get_level(QModelIndex index) -> TreeLevel {
  // root will be an invalid index
  return !index.isValid() ? root_level
         // chords have null parent pointers
         : index.internalPointer() == nullptr ? chord_level
                                              : note_level;
}

auto validate(QWidget *parent_pointer, const nlohmann::json &copied,
              const nlohmann::json_schema::json_validator &validator) -> bool {
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

void copy_text(const std::string &text, const std::string &mime_type) {
  auto *new_data_pointer = std::make_unique<QMimeData>().release();

  Q_ASSERT(new_data_pointer != nullptr);
  new_data_pointer->setData(QString::fromStdString(mime_type),
                            QByteArray::fromStdString(text));

  auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  clipboard_pointer->setMimeData(new_data_pointer);
}

void ChordsModel::check_chord_number(size_t chord_number) const {
  Q_ASSERT(chord_number < chords.size());
}

void ChordsModel::check_new_chord_number(size_t chord_number) const {
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
  auto child_number = to_unsigned(index.row());
  if (get_level(index) == chord_level) {
    return &get_const_chord(child_number);
  }
  return &get_const_chord(to_unsigned(parent(index).row()))
              .get_const_note(child_number);
}

void ChordsModel::mime_type_error(const QMimeData *mime_pointer) {
  Q_ASSERT(mime_pointer != nullptr);
  auto formats = mime_pointer->formats();
  Q_ASSERT(!(formats.empty()));
  std::stringstream stream;
  stream << tr("Cannot paste MIME type \"").toStdString()
         << formats[0].toStdString() << "\"";
  QMessageBox::warning(parent_pointer, tr("MIME type error"),
                       stream.str().c_str());
}

void ChordsModel::throw_parse_error(
    const nlohmann::json::parse_error &parse_error) {
  QMessageBox::warning(parent_pointer, tr("Parsing error"), parse_error.what());
}

void ChordsModel::column_type_error(NoteChordField note_chord_field,
                                    const std::string &type) {
  std::stringstream stream;
  stream << "Cannot paste " << type << " into "
         << get_column_name(note_chord_field) << " column";
  QMessageBox::warning(parent_pointer, tr("Column type error"),
                       tr(stream.str().c_str()));
}

void ChordsModel::add_cell_change(const QModelIndex &index,
                                  const QVariant &new_value) {
  Q_ASSERT(undo_stack_pointer != nullptr);

  auto child_number = to_unsigned(index.row());
  auto note_chord_field = to_note_chord_field(index.column());

  if (get_level(index) == chord_level) {
    undo_stack_pointer->push(
        std::make_unique<ChordCellChange>(this, child_number, note_chord_field,
                                          data(index, Qt::EditRole), new_value)
            .release());
  } else {
    undo_stack_pointer->push(
        std::make_unique<NoteCellChange>(this, child_number, note_chord_field,
                                         to_unsigned(parent(index).row()),
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
  if (parent_level == chord_level && parent_index.column() == symbol_column) {
    return static_cast<int>(
        get_const_chord(to_unsigned(parent_index.row())).notes.size());
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
      symbol_column, nullptr);
}

auto ChordsModel::index(int child_number, int column,
                        const QModelIndex &parent_index) const -> QModelIndex {
  auto positive_child_number = to_unsigned(child_number);
  if (get_level(parent_index) == root_level) {
    return get_chord_index(positive_child_number, to_note_chord_field(column));
  }
  return get_note_index(to_unsigned(parent_index.row()), positive_child_number,
                        to_note_chord_field(column));
}

auto ChordsModel::headerData(int section, Qt::Orientation orientation,
                             int role) const -> QVariant {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole &&
      section != symbol_column) {
    return tr(get_column_name(to_note_chord_field(section)));
  }
  // no horizontal headers
  // no headers for other roles
  return {};
}

auto ChordsModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  auto selectable = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  if (to_note_chord_field(index.column()) == symbol_column) {
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
          get_const_chord(to_unsigned(first_child_number - 1)).beats;
    }

    std::vector<Chord> new_chords;
    for (auto index = 0; index < number_of_children; index = index + 1) {
      new_chords.push_back(template_chord);
    }
    undo_stack_pointer->push(
        std::make_unique<InsertChords>(this, first_child_number, new_chords)
            .release());
  } else {
    auto chord_number = to_unsigned(parent_index.row());
    const auto &parent_chord = get_const_chord(chord_number);

    Note template_note;

    if (first_child_number == 0) {
      template_note.beats = parent_chord.beats;
      template_note.words = parent_chord.words;
    } else {
      const auto &previous_note =
          parent_chord.get_const_note(to_unsigned(first_child_number - 1));

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
    undo_stack_pointer->push(
        std::make_unique<InsertNotes>(this, first_child_number, new_notes,
                                      chord_number)
            .release());
  }
  return true;
}

auto ChordsModel::removeRows(int first_child_number, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  auto positive_first_child_number = to_unsigned(first_child_number);

  auto end_number = first_child_number + number_of_children;
  auto positive_end_number = to_unsigned(end_number);

  Q_ASSERT(undo_stack_pointer != nullptr);

  if (get_level(parent_index) == root_level) {
    check_chord_number(positive_first_child_number);
    check_new_chord_number(positive_end_number);

    undo_stack_pointer->push(
        std::make_unique<RemoveChords>(
            this, positive_first_child_number,
            std::vector<Chord>(chords.cbegin() + first_child_number,
                               chords.cbegin() + end_number))
            .release());
  } else {
    auto chord_number = to_unsigned(parent_index.row());
    const auto &chord = get_const_chord(chord_number);
    undo_stack_pointer->push(
        std::make_unique<RemoveNotes>(
            this, first_child_number,
            chord.copy_notes(positive_first_child_number, positive_end_number),
            chord_number)
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

void ChordsModel::set_note_cell(size_t note_number,
                                NoteChordField note_chord_field,
                                size_t chord_number,
                                const QVariant &new_value) {
  get_chord(chord_number)
      .get_note(note_number)
      .setData(note_chord_field, new_value);
  auto index =
      get_note_index(chord_number, note_number, note_chord_field);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, Qt::EditRole});
}

void ChordsModel::copy_cell(const QModelIndex &index) {
  const auto *note_chord_pointer = get_const_note_chord_pointer(index);
  Q_ASSERT(note_chord_pointer != nullptr);
  const auto *instrument_pointer = note_chord_pointer->instrument_pointer;
  Q_ASSERT(instrument_pointer != nullptr);

  switch (to_note_chord_field(index.column())) {
  case symbol_column: {
    Q_ASSERT(false);
    return;
  };
  case instrument_column: {
    copy_json(nlohmann::json(instrument_pointer->instrument_name),
              INSTRUMENT_MIME);
    return;
  }
  case interval_column: {
    copy_json(note_chord_pointer->interval.json(), INTERVAL_MIME);
    return;
  };
  case beats_column: {
    copy_json(note_chord_pointer->beats.json(), RATIONAL_MIME);
    return;
  };
  case volume_ratio_column: {
    copy_json(note_chord_pointer->volume_ratio.json(), RATIONAL_MIME);
    return;
  };
  case tempo_ratio_column: {
    copy_json(note_chord_pointer->tempo_ratio.json(), RATIONAL_MIME);
    return;
  };
  case words_column: {
    copy_json(nlohmann::json(note_chord_pointer->words), WORDS_MIME);
    return;
  };
  }
}

void ChordsModel::paste_cell(const QModelIndex &index) {
  const auto *mime_data_pointer = get_mime_data_pointer();
  Q_ASSERT(mime_data_pointer != nullptr);

  auto note_chord_field = to_note_chord_field(index.column());

  if (mime_data_pointer->hasFormat(RATIONAL_MIME)) {
    if (note_chord_field != beats_column &&
        note_chord_field != volume_ratio_column &&
        note_chord_field != tempo_ratio_column) {
      column_type_error(note_chord_field, "a rational");
      return;
    }

    auto copied_text = get_copied_text(mime_data_pointer, RATIONAL_MIME);

    nlohmann::json json_value;
    try {
      json_value = nlohmann::json::parse(copied_text);
    } catch (const nlohmann::json::parse_error &parse_error) {
      throw_parse_error(parse_error);
      return;
    }

    static const nlohmann::json_schema::json_validator rational_validator =
        []() {
          auto rational_schema = get_rational_schema("rational");
          rational_schema["$schema"] =
              "http://json-schema.org/draft-07/schema#";
          rational_schema["title"] = "Interval";
          return nlohmann::json_schema::json_validator(rational_schema);
        }();

    if (!(validate(parent_pointer, json_value, rational_validator))) {
      return;
    }

    add_cell_change(index, QVariant::fromValue(Rational(json_value)));
  } else if (mime_data_pointer->hasFormat(INTERVAL_MIME)) {
    if (note_chord_field != interval_column) {
      column_type_error(note_chord_field, "an interval");
      return;
    }

    auto copied_text = get_copied_text(mime_data_pointer, INTERVAL_MIME);

    nlohmann::json json_value;
    try {
      json_value = nlohmann::json::parse(copied_text);
    } catch (const nlohmann::json::parse_error &parse_error) {
      throw_parse_error(parse_error);
      return;
    }

    static const nlohmann::json_schema::json_validator interval_validator =
        []() {
          auto interval_schema = get_interval_schema();
          interval_schema["$schema"] =
              "http://json-schema.org/draft-07/schema#";
          interval_schema["title"] = "Interval";
          return interval_schema;
        }();

    if (!(validate(parent_pointer, json_value, interval_validator))) {
      return;
    }

    add_cell_change(index, QVariant::fromValue(Interval(json_value)));
  } else if (mime_data_pointer->hasFormat(INSTRUMENT_MIME)) {
    if (note_chord_field != instrument_column) {
      column_type_error(note_chord_field, "an instrument");
      return;
    }

    auto copied_text = get_copied_text(mime_data_pointer, INSTRUMENT_MIME);

    nlohmann::json json_value;
    try {
      json_value = nlohmann::json::parse(copied_text);
    } catch (const nlohmann::json::parse_error &parse_error) {
      throw_parse_error(parse_error);
      return;
    }

    static const nlohmann::json_schema::json_validator instrument_validator =
        []() {
          auto instrument_schema = get_instrument_schema();
          instrument_schema["$schema"] =
              "http://json-schema.org/draft-07/schema#";
          instrument_schema["title"] = "Interval";
          return nlohmann::json_schema::json_validator(instrument_schema);
        }();

    if (!(validate(parent_pointer, json_value, instrument_validator))) {
      return;
    }
    add_cell_change(index, QVariant::fromValue(get_instrument_pointer(
                               json_value.get<std::string>())));
  } else if (mime_data_pointer->hasFormat(WORDS_MIME)) {
    if (note_chord_field != words_column) {
      column_type_error(note_chord_field, "words");
      return;
    }

    auto copied_text = get_copied_text(mime_data_pointer, WORDS_MIME);

    nlohmann::json json_value;
    try {
      json_value = nlohmann::json::parse(copied_text);
    } catch (const nlohmann::json::parse_error &parse_error) {
      throw_parse_error(parse_error);
      return;
    }

    static const nlohmann::json_schema::json_validator words_validator(
        nlohmann::json({{"$schema", "http://json-schema.org/draft-07/schema#"},
                        {"type", "string"},
                        {"title", "words"},
                        {"description", "the words"}}));

    if (!(validate(parent_pointer, json_value, words_validator))) {
      return;
    }
    add_cell_change(index, QVariant(json_value.get<std::string>().c_str()));
  } else {
    mime_type_error(mime_data_pointer);
  }
}

void ChordsModel::insert_notes(size_t first_note_number,
                               const std::vector<Note> &new_notes,
                               size_t chord_number) {
  beginInsertRows(get_chord_index(chord_number),
                  static_cast<int>(first_note_number),
                  static_cast<int>(first_note_number + new_notes.size()) - 1);
  get_chord(chord_number).insert_notes(first_note_number, new_notes);
  endInsertRows();
};

void ChordsModel::insert_chords(size_t first_chord_number,
                                const std::vector<Chord> &new_chords) {
  auto int_first_chord_number = static_cast<int>(first_chord_number);

  check_new_chord_number(first_chord_number);

  beginInsertRows(QModelIndex(), int_first_chord_number,
                  static_cast<int>(first_chord_number + new_chords.size()) - 1);
  chords.insert(chords.begin() + int_first_chord_number, new_chords.begin(),
                new_chords.end());
  endInsertRows();
}

void ChordsModel::remove_notes(size_t first_note_number, size_t number_of_notes,
                               size_t chord_number) {
  beginRemoveRows(get_chord_index(chord_number),
                  static_cast<int>(first_note_number),
                  static_cast<int>(first_note_number + number_of_notes) - 1);
  get_chord(chord_number).remove_notes(first_note_number, number_of_notes);
  endRemoveRows();
}

void ChordsModel::remove_chords(size_t first_chord_number,
                                size_t number_of_chords) {
  auto end_number = first_chord_number + number_of_chords;
  auto int_first_chord_number = static_cast<int>(first_chord_number);
  auto int_end_number = static_cast<int>(end_number);

  beginRemoveRows(QModelIndex(), int_first_chord_number, int_end_number - 1);
  check_chord_number(first_chord_number);
  check_new_chord_number(end_number);
  chords.erase(chords.begin() + int_first_chord_number,
               chords.begin() + int_end_number);
  endRemoveRows();
}

auto ChordsModel::json_copy_chords(size_t first_chord_number,
                                   size_t number_of_chords) const
    -> nlohmann::json {
  nlohmann::json json_chords;

  check_chord_number(first_chord_number);

  auto end_number = first_chord_number + number_of_chords;
  check_new_chord_number(end_number);

  std::transform(chords.cbegin() + static_cast<int>(first_chord_number),
                 chords.cbegin() + static_cast<int>(end_number),
                 std::back_inserter(json_chords),
                 [](const Chord &chord) { return chord.json(); });
  return json_chords;
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

void ChordsModel::insert_json_notes(size_t first_note_number,
                                    const nlohmann::json &json_notes,
                                    size_t chord_number) {
  beginInsertRows(get_chord_index(chord_number),
                  static_cast<int>(first_note_number),
                  static_cast<int>(first_note_number + json_notes.size()) - 1);
  get_chord(chord_number).insert_json_notes(first_note_number, json_notes);
  endInsertRows();
}

void ChordsModel::load_chords(const nlohmann::json &json_chords) {
  if (!chords.empty()) {
    remove_chords(0, chords.size());
  }
  insert_json_chords(0, json_chords);
}

void ChordsModel::copy_rows(size_t first_child_number,
                            size_t number_of_children,
                            const QModelIndex &parent_index) const {
  if (get_level(parent_index) == root_level) {
    copy_json(json_copy_chords(first_child_number, number_of_children),
              CHORDS_MIME);
  } else {
    copy_json(get_const_chord(to_unsigned(parent_index.row()))
                  .json_copy_notes(first_child_number, number_of_children),
              NOTES_MIME);
  }
}

void ChordsModel::paste_rows(size_t first_child_number,
                             const QModelIndex &parent_index) {
  auto parent_level = get_level(parent_index);

  const auto *mime_data_pointer = get_mime_data_pointer();
  Q_ASSERT(mime_data_pointer != nullptr);

  if (mime_data_pointer->hasFormat(CHORDS_MIME)) {
    if (parent_level != root_level) {
      QMessageBox::warning(parent_pointer, tr("Type error"),
                           tr("Cannot paste chords into another chord!"));
      return;
    }
    auto text = get_copied_text(mime_data_pointer, CHORDS_MIME);
    nlohmann::json json_chords;
    try {
      json_chords = nlohmann::json::parse(text);
    } catch (const nlohmann::json::parse_error &parse_error) {
      throw_parse_error(parse_error);
      return;
    }

    static const nlohmann::json_schema::json_validator chords_validator(
        nlohmann::json({
            {"$schema", "http://json-schema.org/draft-07/schema#"},
            {"title", "Chords"},
            {"description", "a list of chords"},
            {"type", "array"},
            {"items", get_chord_schema()},
        }));

    if (!(validate(parent_pointer, json_chords, chords_validator))) {
      return;
    }

    Q_ASSERT(undo_stack_pointer != nullptr);
    undo_stack_pointer->push(std::make_unique<InsertJsonChords>(
                                 this, first_child_number, json_chords)
                                 .release());
  } else if (mime_data_pointer->hasFormat(NOTES_MIME)) {
    if (parent_level != chord_level) {
      QMessageBox::warning(parent_pointer, tr("Type error"),
                           tr("Can only paste notes into a chord!"));
      return;
    }
    auto text = get_copied_text(mime_data_pointer, NOTES_MIME);
    nlohmann::json json_notes;
    try {
      json_notes = nlohmann::json::parse(text);
    } catch (const nlohmann::json::parse_error &parse_error) {
      throw_parse_error(parse_error);
      return;
    }
    static const nlohmann::json_schema::json_validator notes_validator(
        nlohmann::json({{"$schema", "http://json-schema.org/draft-07/schema#"},
                        {"type", "array"},
                        {"title", "Notes"},
                        {"description", "the notes"},
                        {"items", get_note_schema()}}));
    if (!(validate(parent_pointer, json_notes, notes_validator))) {
      return;
    }

    Q_ASSERT(undo_stack_pointer != nullptr);

    Q_ASSERT(undo_stack_pointer != nullptr);
    undo_stack_pointer->push(
        std::make_unique<InsertJsonNotes>(this, first_child_number, json_notes,
                                          to_unsigned(parent_index.row()))
            .release());
  } else {
    mime_type_error(mime_data_pointer);
  }
}
