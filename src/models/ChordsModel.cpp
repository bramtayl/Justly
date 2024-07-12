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
#include <initializer_list>
#include <iomanip>
#include <iterator>
#include <memory>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "changes/CellChange.hpp"
#include "changes/InsertJson.hpp"
#include "changes/InsertRemoveChords.hpp"
#include "changes/InsertRemoveNotes.hpp"
#include "justly/CellIndex.hpp"
#include "justly/Chord.hpp"
#include "justly/Instrument.hpp"
#include "justly/Interval.hpp"
#include "justly/Note.hpp"
#include "justly/NoteChord.hpp"
#include "justly/NoteChordField.hpp"
#include "justly/Rational.hpp"
#include "justly/TreeLevel.hpp"
#include "justly/public_constants.hpp"
#include "other/json.hpp"

auto get_column_name(NoteChordField note_chord_field) {
  switch (note_chord_field) {
    case symbol_column:
      Q_ASSERT(false);
      return "";
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
    Q_ASSERT(parent_pointer != nullptr);
    error_message << error.what();
    QMessageBox::warning(parent_pointer, QWidget::tr("Schema error"),
                         QString::fromStdString(error_message.str()));
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

auto ChordsModel::make_parent_index(int parent_number) const -> QModelIndex {
  return parent_number == -1
             // for root, use an empty index
             ? QModelIndex()
             // for chords, the parent pointer is null
             : createIndex(parent_number, symbol_column, nullptr);
}

auto ChordsModel::to_cell_index(const QModelIndex &index) const -> CellIndex {
  auto row = index.row();
  Q_ASSERT(row >= 0);

  return {parent(index).row(), static_cast<size_t>(row),
          to_note_chord_field(index.column())};
}

[[nodiscard]] auto ChordsModel::get_const_note_chord_pointer(
    int parent_number, size_t child_number) const -> const NoteChord * {
  if (parent_number == -1) {
    Q_ASSERT(child_number < chords.size());
    return &chords[child_number];
  }
  const auto &notes = chords[verify_chord_number(parent_number)].notes;
  Q_ASSERT(child_number < notes.size());
  return &notes[child_number];
}

auto ChordsModel::verify_chord_number(int chord_number) const -> size_t {
  Q_ASSERT(chord_number >= 0);
  auto positive_chord_number = static_cast<size_t>(chord_number);
  Q_ASSERT(positive_chord_number < chords.size());
  return positive_chord_number;
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
  Q_ASSERT(parent_pointer != nullptr);
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
  undo_stack_pointer->push(
      std::make_unique<CellChange>(this, to_cell_index(index),
                                   data(index, Qt::EditRole), new_value)
          .release());
}

void ChordsModel::add_insert_json_change(size_t first_child_number,
                                         const nlohmann::json &json_children,
                                         int parent_number) {
  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->push(
      std::make_unique<InsertJson>(this, first_child_number, json_children,
                                   parent_number)
          .release());
}

void ChordsModel::add_insert_remove_notes_change(
    size_t first_child_number, const std::vector<Note> &new_notes,
    int parent_number, bool is_insert) {
  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->push(
      std::make_unique<InsertRemoveNotes>(this, first_child_number, new_notes,
                                          parent_number, is_insert)
          .release());
}

void ChordsModel::add_insert_remove_chords_change(
    size_t first_child_number, const std::vector<Chord> &new_chords,
    bool is_insert) {
  Q_ASSERT(undo_stack_pointer != nullptr);
  undo_stack_pointer->push(std::make_unique<InsertRemoveChords>(
                               this, first_child_number, new_chords, is_insert)
                               .release());
}

void ChordsModel::remove_notes_directly(size_t first_child_number,
                                        size_t number_of_children,
                                        int parent_number) {
  auto end_number = first_child_number + number_of_children;
  auto int_first_child_number = static_cast<int>(first_child_number);
  auto int_end_number = static_cast<int>(end_number);

  beginRemoveRows(make_parent_index(parent_number), int_first_child_number,
                  int_end_number - 1);
  auto &notes = chords[verify_chord_number(parent_number)].notes;
  auto notes_size = notes.size();

  Q_ASSERT(first_child_number < notes_size);
  Q_ASSERT(end_number <= notes_size);

  notes.erase(notes.begin() + int_first_child_number,
              notes.begin() + int_end_number);
  endRemoveRows();
}

void ChordsModel::remove_chords_directly(size_t first_child_number,
                                         size_t number_of_children) {
  auto end_number = first_child_number + number_of_children;
  auto int_first_child_number = static_cast<int>(first_child_number);
  auto int_end_number = static_cast<int>(end_number);

  beginRemoveRows(make_parent_index(-1), int_first_child_number,
                  int_end_number - 1);
  auto chords_size = chords.size();
  Q_ASSERT(first_child_number < chords_size);
  Q_ASSERT(end_number <= chords_size);
  chords.erase(chords.begin() + int_first_child_number,
               chords.begin() + int_end_number);
  endRemoveRows();
}

ChordsModel::ChordsModel(QUndoStack *undo_stack_pointer_input,
                         QWidget *parent_pointer_input)
    : QAbstractItemModel(parent_pointer_input),
      parent_pointer(parent_pointer_input),
      undo_stack_pointer(undo_stack_pointer_input) {}

auto ChordsModel::get_index(size_t child_number, int parent_number,
                            NoteChordField note_chord_field) const
    -> QModelIndex {
  return createIndex(static_cast<int>(child_number), note_chord_field,
                     parent_number >= 0
                         ? &chords[verify_chord_number(parent_number)]
                         : nullptr);
}

auto ChordsModel::rowCount(const QModelIndex &parent_index) const -> int {
  auto parent_level = get_level(parent_index);
  auto chords_size = chords.size();
  if (parent_level == root_level) {
    return static_cast<int>(chords_size);
  }
  // only nest into the symbol column
  if (parent_level == chord_level && parent_index.column() == symbol_column) {
    return static_cast<int>(
        chords[verify_chord_number(parent_index.row())].notes.size());
  }
  // notes have no children
  return 0;
}

auto ChordsModel::columnCount(const QModelIndex & /*parent*/) const -> int {
  return NOTE_CHORD_COLUMNS;
}

// get the parent index
auto ChordsModel::parent(const QModelIndex &index) const -> QModelIndex {
  auto level = get_level(index);
  if (level == root_level) {
    Q_ASSERT(false);
    return {};
  }
  if (level == chord_level) {
    return {};
  }
  return createIndex(
      static_cast<int>(std::distance(
          chords.data(), static_cast<const Chord *>(index.internalPointer()))),
      symbol_column, nullptr);
}

// get a child index
auto ChordsModel::index(int child_number, int column,
                        const QModelIndex &parent_index) const -> QModelIndex {
  return get_index(child_number, parent_index.row(),
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
  auto cell_index = to_cell_index(index);
  const auto *note_chord_pointer = get_const_note_chord_pointer(
      cell_index.parent_number, cell_index.child_number);
  Q_ASSERT(note_chord_pointer != nullptr);
  return note_chord_pointer->data(cell_index.note_chord_field, role);
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
  auto parent_number = parent_index.row();

  Q_ASSERT(undo_stack_pointer != nullptr);

  if (parent_number == -1) {
    Chord template_chord;
    if (first_child_number > 0) {
      template_chord.beats =
          chords[verify_chord_number(first_child_number - 1)].beats;
    }

    std::vector<Chord> new_chords;
    for (auto index = 0; index < number_of_children; index = index + 1) {
      new_chords.push_back(template_chord);
    }
    add_insert_remove_chords_change(first_child_number, new_chords, true);
  } else {
    const auto &parent_chord = chords[verify_chord_number(parent_number)];

    Note template_note;

    if (first_child_number == 0) {
      template_note.beats = parent_chord.beats;
      template_note.words = parent_chord.words;
    } else {
      const auto &notes = parent_chord.notes;
      auto previous_note_number = first_child_number - 1;

      Q_ASSERT(0 <= previous_note_number);
      Q_ASSERT(static_cast<size_t>(previous_note_number) < notes.size());
      const auto &previous_note = notes[previous_note_number];

      template_note.beats = previous_note.beats;
      template_note.volume_ratio = previous_note.volume_ratio;
      template_note.tempo_ratio = previous_note.tempo_ratio;
      template_note.words = previous_note.words;
    }

    std::vector<Note> new_notes;
    for (auto index = 0; index < number_of_children; index = index + 1) {
      new_notes.push_back(template_note);
    }
    add_insert_remove_notes_change(first_child_number, new_notes, parent_number,
                                   true);
  }
  return true;
}

auto ChordsModel::removeRows(int first_child_number, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  auto parent_number = parent_index.row();

  Q_ASSERT(first_child_number >= 0);
  auto positive_first_child_number = static_cast<size_t>(first_child_number);

  Q_ASSERT(number_of_children >= 0);
  auto end_number =
      first_child_number + static_cast<size_t>(number_of_children);
  auto int_end_number = static_cast<int>(end_number);

  Q_ASSERT(undo_stack_pointer != nullptr);

  if (parent_number == -1) {
    auto chords_size = chords.size();

    Q_ASSERT(positive_first_child_number < chords_size);
    Q_ASSERT(end_number <= chords_size);

    add_insert_remove_chords_change(
        positive_first_child_number,
        std::vector<Chord>(chords.cbegin() + first_child_number,
                           chords.cbegin() + int_end_number),
        false);
  } else {
    const auto &notes = chords[verify_chord_number(parent_number)].notes;

    auto notes_size = notes.size();

    Q_ASSERT(positive_first_child_number < notes_size);
    Q_ASSERT(end_number <= notes_size);
    add_insert_remove_notes_change(
        first_child_number,
        std::vector<Note>(notes.cbegin() + first_child_number,
                          notes.cbegin() + int_end_number),
        parent_number, false);
  }
  return true;
}

void ChordsModel::set_cell(const CellIndex &cell_index,
                           const QVariant &new_value) {
  auto parent_number = cell_index.parent_number;
  auto child_number = cell_index.child_number;
  auto note_chord_field = cell_index.note_chord_field;

  if (parent_number == -1) {
    Q_ASSERT(child_number < chords.size());
    chords[child_number].setData(note_chord_field, new_value);
  } else {
    auto &notes = chords[verify_chord_number(parent_number)].notes;
    Q_ASSERT(child_number < notes.size());
    notes[child_number].setData(note_chord_field, new_value);
  }
  auto index = get_index(child_number, parent_number, note_chord_field);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, Qt::EditRole});
}

void ChordsModel::copy_cell(const QModelIndex &index) {
  auto cell_index = to_cell_index(index);

  const auto *note_chord_pointer = get_const_note_chord_pointer(
      cell_index.parent_number, cell_index.child_number);
  Q_ASSERT(note_chord_pointer != nullptr);
  const auto *instrument_pointer = note_chord_pointer->instrument_pointer;
  Q_ASSERT(instrument_pointer != nullptr);

  switch (cell_index.note_chord_field) {
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

void ChordsModel::insert_remove_notes(size_t first_child_number,
                                      const std::vector<Note> &new_notes,
                                      int parent_number, bool should_insert) {
  auto number_of_children = new_notes.size();
  if (should_insert) {
    auto &notes = chords[verify_chord_number(parent_number)].notes;
    auto int_first_child_number = static_cast<int>(first_child_number);

    beginInsertRows(
        make_parent_index(parent_number), int_first_child_number,
        static_cast<int>(first_child_number + number_of_children) - 1);
    notes.insert(notes.begin() + int_first_child_number, new_notes.begin(),
                 new_notes.end());
    endInsertRows();
  } else {
    remove_notes_directly(first_child_number, number_of_children,
                          parent_number);
  }
};

void ChordsModel::insert_remove_chords(size_t first_child_number,
                                       const std::vector<Chord> &new_chords,
                                       bool should_insert) {
  auto number_of_children = new_chords.size();
  if (should_insert) {
    Q_ASSERT(first_child_number <= chords.size());
    auto int_first_child_number = static_cast<int>(first_child_number);
    beginInsertRows(
        make_parent_index(-1), int_first_child_number,
        static_cast<int>(first_child_number + number_of_children) - 1);
    chords.insert(chords.begin() + int_first_child_number, new_chords.begin(),
                  new_chords.end());
    endInsertRows();
  } else {
    remove_chords_directly(first_child_number, number_of_children);
  }
}

void ChordsModel::remove_directly(size_t first_child_number,
                                  size_t number_of_children,
                                  int parent_number) {
  if (parent_number == -1) {
    remove_chords_directly(first_child_number, number_of_children);
  } else {
    remove_notes_directly(first_child_number, number_of_children,
                          parent_number);
  }
}

void ChordsModel::insert_json(size_t first_child_number,
                              const nlohmann::json &json_children,
                              int parent_number) {
  auto end_number = first_child_number + json_children.size();
  beginInsertRows(make_parent_index(parent_number),
                  static_cast<int>(first_child_number),
                  static_cast<int>(end_number) - 1);
  if (parent_number == -1) {
    insert_from_json(chords, first_child_number, json_children);
  } else {
    insert_from_json(chords[verify_chord_number(parent_number)].notes,
                     first_child_number, json_children);
  }
  endInsertRows();
}

void ChordsModel::load_chords(const nlohmann::json &json_song) {
  beginResetModel();
  chords.clear();
  if (json_song.contains("chords")) {
    insert_from_json(chords, 0, json_song["chords"]);
  }
  endResetModel();
}

void ChordsModel::copy_rows(size_t first_child_number,
                            size_t number_of_children, int parent_number) {
  if (parent_number == -1) {
    // or root
    copy_json(objects_to_json(chords, first_child_number, number_of_children),
              CHORDS_MIME);
  } else {
    // for chord
    copy_json(objects_to_json(chords[verify_chord_number(parent_number)].notes,
                              first_child_number, number_of_children),
              NOTES_MIME);
  }
}

void ChordsModel::paste_rows(int first_child_number,
                             const QModelIndex &parent_index) {
  auto parent_number = parent_index.row();

  const auto *mime_data_pointer = get_mime_data_pointer();
  Q_ASSERT(mime_data_pointer != nullptr);

  if (mime_data_pointer->hasFormat(CHORDS_MIME)) {
    if (parent_number != -1) {
      QMessageBox::warning(parent_pointer, tr("Type error"),
                           tr("Cannot paste chords into another chord!"));
      return;
    }
    auto text = get_copied_text(mime_data_pointer, CHORDS_MIME);
    nlohmann::json json_children;
    try {
      json_children = nlohmann::json::parse(text);
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

    if (!(validate(parent_pointer, json_children, chords_validator))) {
      return;
    }

    add_insert_json_change(first_child_number, json_children, parent_number);
  } else if (mime_data_pointer->hasFormat(NOTES_MIME)) {
    if (parent_number == -1) {
      QMessageBox::warning(parent_pointer, tr("Type error"),
                           tr("Can only paste notes into a chord!"));
      return;
    }
    auto text = get_copied_text(mime_data_pointer, NOTES_MIME);
    nlohmann::json json_children;
    try {
      json_children = nlohmann::json::parse(text);
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
    if (!(validate(parent_pointer, json_children, notes_validator))) {
      return;
    }

    Q_ASSERT(undo_stack_pointer != nullptr);

    add_insert_json_change(first_child_number, json_children, parent_number);
  } else {
    mime_type_error(mime_data_pointer);
  }
}
