#include "justly/ChordsModel.hpp"

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractIt...
#include <qassert.h>             // for Q_ASSERT
#include <qbytearray.h>          // for QByteArray
#include <qclipboard.h>          // for QClipboard
#include <qflags.h>              // for QFlags
#include <qguiapplication.h>     // for QGuiApplication
#include <qmimedata.h>           // for QMimeData
#include <qnamespace.h>          // for EditRole, DisplayRole
#include <qtmetamacros.h>        // for emit
#include <qundostack.h>          // for QUndoStack
#include <qvariant.h>            // for QVariant

#include <algorithm>                         // for transform
#include <cstddef>                           // for size_t
#include <initializer_list>                  // for initializer_list
#include <iomanip>                           // for operator<<, setw
#include <map>                               // for operator!=, operator==
#include <memory>                            // for make_unique, __unique_ptr_t
#include <nlohmann/detail/json_pointer.hpp>  // for json_pointer<>::string_t
#include <nlohmann/detail/json_ref.hpp>      // for json_ref
#include <nlohmann/json-schema.hpp>          // for json_validator
#include <nlohmann/json.hpp>                 // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>             // for json
#include <ostream>                           // for stringstream, basic_ostream
#include <string>                            // for string, char_traits, bas...
#include <vector>                            // for vector

#include "changes/CellChange.hpp"          // for CellChange
#include "changes/InsertRemoveChange.hpp"  // for InsertRemoveChange
#include "json/JsonErrorHandler.hpp"       // for JsonErrorHandler, show_p...
#include "json/json.hpp"                   // for insert_from_json, object...
#include "justly/CellIndex.hpp"            // for CellIndex
#include "justly/Chord.hpp"                // for Chord, get_chord_schema
#include "justly/CopyType.hpp"             // for rational_copy, instrumen...
#include "justly/Instrument.hpp"           // for get_instrument_pointer
#include "justly/Interval.hpp"             // for Interval
#include "justly/Note.hpp"                 // for Note, get_note_schema
#include "justly/NoteChord.hpp"            // for NoteChord
#include "justly/NoteChordField.hpp"       // for to_note_chord_field, sym...
#include "justly/Rational.hpp"             // for Rational
#include "justly/public_constants.hpp"     // for NOTE_CHORD_COLUMNS

class QObject;  // lines 19-19

auto get_mime_data_pointer() -> const QMimeData * {
  auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  return clipboard_pointer->mimeData();
}

auto get_copy_type(NoteChordField note_chord_field) -> CopyType {
  switch (note_chord_field) {
    case symbol_column: {
      Q_ASSERT(false);
      return {};
    }
    case instrument_column: {
      return instrument_copy;
    }
    case beats_column: {
      return rational_copy;
    }
    case interval_column: {
      return interval_copy;
    }
    case volume_ratio_column: {
      return rational_copy;
    }
    case words_column: {
      return words_copy;
    }
    case tempo_ratio_column: {
      return rational_copy;
    }
  }
}

void copy_json(const nlohmann::json &copied) {
  auto *new_data_pointer = std::make_unique<QMimeData>().release();
  std::stringstream json_text;

  json_text << std::setw(4) << copied;

  Q_ASSERT(new_data_pointer != nullptr);
  new_data_pointer->setData("application/json",
                            QByteArray::fromStdString(json_text.str()));

  auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  clipboard_pointer->setMimeData(new_data_pointer);
}

auto ChordsModel::make_chord_index(int parent_number) const -> QModelIndex {
  return parent_number == -1
             // for root, use an empty index
             ? QModelIndex()
             // for chords, the parent pointer is null
             : createIndex(parent_number, symbol_column, nullptr);
}

auto ChordsModel::get_index(int parent_number, size_t child_number,
                            NoteChordField note_chord_field) const
    -> QModelIndex {
  auto int_child_number = static_cast<int>(child_number);

  if (parent_number == -1) {
    return createIndex(int_child_number, note_chord_field, nullptr);
  }

  Q_ASSERT(0 <= parent_number);
  Q_ASSERT(static_cast<size_t>(parent_number) < chords.size());

  return createIndex(int_child_number, note_chord_field,
                     &chords[parent_number]);
}

auto ChordsModel::to_cell_index(const QModelIndex &index) const -> CellIndex {
  auto level = get_level(index);
  if (level == root_level) {
    Q_ASSERT(false);
    return {-1, 0};
  }
  auto row = index.row();
  Q_ASSERT(row >= 0);

  return {level == chord_level ? -1 : parent(index).row(),
          static_cast<size_t>(row), to_note_chord_field(index.column())};
}

ChordsModel::ChordsModel(QUndoStack *undo_stack_pointer_input,
                         QWidget *parent_pointer_input)
    : QAbstractItemModel(parent_pointer_input),
      parent_pointer(parent_pointer_input),
      undo_stack_pointer(undo_stack_pointer_input) {}

void ChordsModel::load_chords(const nlohmann::json &json_song) {
  beginResetModel();
  chords.clear();
  if (json_song.contains("chords")) {
    insert_from_json(chords, 0, json_song["chords"]);
  }
  endResetModel();
};

auto ChordsModel::rowCount(const QModelIndex &parent_index) const -> int {
  auto parent_level = get_level(parent_index);
  auto chords_size = chords.size();
  if (parent_level == root_level) {
    return static_cast<int>(chords_size);
  }
  if (parent_level == chord_level) {
    if (parent_index.column() != symbol_column) {
      return 0;
    }
    auto chord_number = parent_index.column();
    Q_ASSERT(0 <= chord_number);
    Q_ASSERT(static_cast<size_t>(chord_number) < chords_size);
    const auto &chord = chords[chord_number];
    return static_cast<int>(chord.notes.size());
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
  auto *chord_pointer = static_cast<Chord *>(index.internalPointer());
  auto chord_number = -1;
  for (size_t maybe_chord_number = 0; maybe_chord_number < chords.size();
       maybe_chord_number = maybe_chord_number + 1) {
    if (chord_pointer == &chords[maybe_chord_number]) {
      chord_number = static_cast<int>(maybe_chord_number);
    }
  }
  Q_ASSERT(chord_number >= 0);
  return createIndex(chord_number, symbol_column, nullptr);
}

// get a child index
auto ChordsModel::index(int child_number, int column,
                        const QModelIndex &parent_index) const -> QModelIndex {
  return get_index(to_parent_number(parent_index), child_number,
                   to_note_chord_field(column));
}

auto ChordsModel::headerData(int section, Qt::Orientation orientation,
                             int role) const -> QVariant {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch (to_note_chord_field(section)) {
      case symbol_column:
        return {};
      case interval_column:
        return tr("Interval");
      case beats_column:
        return tr("Beats");
      case volume_ratio_column:
        return tr("Volume ratio");
      case tempo_ratio_column:
        return tr("Tempo ratio");
      case words_column:
        return tr("Words");
      case instrument_column:
        return tr("Instrument");
    }
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
  return get_const_note_chord_pointer(cell_index.parent_number,
                                      cell_index.child_number)
      ->data(cell_index.note_chord_field, role);
}

auto ChordsModel::insertRows(int first_child_number, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  auto parent_number = to_parent_number(parent_index);
  nlohmann::json json_objects = nlohmann::json::array();

  auto chords_size = chords.size();
  if (parent_number == -1) {
    Chord template_chord;
    if (first_child_number > 0) {
      auto previous_number = first_child_number - 1;
      Q_ASSERT(0 <= previous_number);
      Q_ASSERT(static_cast<size_t>(previous_number) <= chords_size);
      template_chord.beats = chords[previous_number].beats;
    }
    for (auto index = 0; index < number_of_children; index = index + 1) {
      json_objects.emplace_back(template_chord.json());
    }
  } else {
    Q_ASSERT(0 <= parent_number);
    Q_ASSERT(static_cast<size_t>(parent_number) <= chords_size);
    const auto &parent_chord = chords[parent_number];

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
    for (auto index = 0; index < number_of_children; index = index + 1) {
      json_objects.emplace_back(template_note.json());
    }
  }
  undo_stack_pointer->push(
      std::make_unique<InsertRemoveChange>(this, first_child_number,
                                           json_objects, parent_number, true)
          .release());
  return true;
}

auto ChordsModel::removeRows(int first_child_number, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  auto parent_number = to_parent_number(parent_index);
  undo_stack_pointer->push(
      std::make_unique<InsertRemoveChange>(
          this, first_child_number,
          copy_rows_to(first_child_number, number_of_children, parent_number),
          parent_number, false)
          .release());
  return true;
}

auto ChordsModel::setData(const QModelIndex &index, const QVariant &new_value,
                          int role) -> bool {
  if (role != Qt::EditRole) {
    return false;
  }
  undo_stack_pointer->push(
      std::make_unique<CellChange>(this, to_cell_index(index),
                                   data(index, Qt::EditRole), new_value)
          .release());
  return true;
}

void ChordsModel::insert_remove_directly(size_t first_child_number,
                                  const nlohmann::json &json_children,
                                  int parent_number, bool should_insert) {
  if (should_insert) {
    beginInsertRows(
        make_chord_index(parent_number), static_cast<int>(first_child_number),
        static_cast<int>(first_child_number + json_children.size()) - 1);
    if (parent_number == -1) {
      insert_from_json(chords, first_child_number, json_children);
    } else {
      Q_ASSERT(0 <= parent_number);
      Q_ASSERT(static_cast<size_t>(parent_number) < chords.size());
      // for a chord
      insert_from_json(chords[parent_number].notes, first_child_number,
                      json_children);
    }
    endInsertRows();
  } else {
    auto number_of_children = json_children.size();
    beginRemoveRows(
        make_chord_index(parent_number), static_cast<int>(first_child_number),
        static_cast<int>(first_child_number + number_of_children) - 1);

    auto chords_size = chords.size();

    auto int_first_child_number = static_cast<int>(first_child_number);

    auto end_number = first_child_number + number_of_children;
    auto int_end_number = static_cast<int>(end_number);

    if (parent_number == -1) {
      // for root
      Q_ASSERT(first_child_number < chords.size());
      Q_ASSERT(end_number <= chords_size);
      chords.erase(chords.begin() + int_first_child_number,
                  chords.begin() + int_end_number);
    } else {
      // for a chord
      Q_ASSERT(0 <= parent_number);
      Q_ASSERT(static_cast<size_t>(parent_number) < chords_size);
      auto &chord = chords[parent_number];

      auto &notes = chord.notes;
      auto notes_size = notes.size();

      Q_ASSERT(first_child_number < notes_size);

      Q_ASSERT(0 < end_number);
      Q_ASSERT(end_number <= notes_size);

      notes.erase(notes.begin() + int_first_child_number,
                  notes.begin() + int_end_number);
    }
    endRemoveRows();
  }
}

void ChordsModel::set_cell_directly(const CellIndex &cell_index,
                                    const QVariant &new_value) {
  auto parent_number = cell_index.parent_number;
  auto child_number = cell_index.child_number;
  auto note_chord_field = cell_index.note_chord_field;

  NoteChord* note_chord_pointer;

  if (parent_number == -1) {
    Q_ASSERT(child_number < chords.size());
    note_chord_pointer = &chords[child_number];
  } else {
    Q_ASSERT(parent_number >= 0);
    Q_ASSERT(static_cast<size_t>(parent_number) < chords.size());
    auto &chord = chords[parent_number];

    auto &notes = chord.notes;
    Q_ASSERT(child_number < notes.size());
    note_chord_pointer = &notes[child_number];
  }

  note_chord_pointer->setData(note_chord_field, new_value);
  auto index = get_index(parent_number, child_number, note_chord_field);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, Qt::EditRole});
}

auto to_parent_number(const QModelIndex &index) -> int {
  auto level = get_level(index);
  if (level == root_level) {
    return -1;
  }
  Q_ASSERT(level == chord_level);
  return index.row();
}

void ChordsModel::paste_rows(int first_child_number,
                             const QModelIndex &parent_index) {
  const auto *mime_data_pointer = get_mime_data_pointer();
  Q_ASSERT(mime_data_pointer != nullptr);
  if (mime_data_pointer->hasFormat("application/json")) {
    paste_rows_text(first_child_number,
                    mime_data_pointer->data("application/json").toStdString(),
                    parent_index);
  }
}

void ChordsModel::paste_cell(const QModelIndex &index) {
  const auto *mime_data_pointer = get_mime_data_pointer();

  if (mime_data_pointer->hasFormat("application/json")) {
    auto text = mime_data_pointer->data("application/json").toStdString();

    auto cell_index = to_cell_index(index);
    auto note_chord_field = cell_index.note_chord_field;

    nlohmann::json json_value;
    try {
      json_value = nlohmann::json::parse(text);
    } catch (const nlohmann::json::parse_error &parse_error) {
      show_parse_error(parent_pointer, parse_error.what());
      return;
    }

    auto copy_type = get_copy_type(note_chord_field);

    // TODO: verify json cell

    QVariant new_value;
    switch (copy_type) {
      case instrument_copy: {
        Q_ASSERT(json_value.is_string());
        new_value = QVariant::fromValue(
            get_instrument_pointer(json_value.get<std::string>()));
        break;
      }
      case rational_copy: {
        new_value = QVariant::fromValue(Rational(json_value));
        break;
      }
      case interval_copy: {
        new_value = QVariant::fromValue(Interval(json_value));
        break;
      }
      case words_copy: {
        Q_ASSERT(json_value.is_string());
        new_value = QVariant(json_value.get<std::string>().c_str());
        break;
      }
      default: {
        Q_ASSERT(false);
        break;
      }
    }
    Q_ASSERT(undo_stack_pointer != nullptr);
    undo_stack_pointer->push(
        std::make_unique<CellChange>(this, cell_index,
                                     data(index, Qt::EditRole), new_value)
            .release());
  }
}

// TODO: consider passing a parent number instead
void ChordsModel::paste_rows_text(int first_child_number,
                                  const std::string &text,
                                  const QModelIndex &parent_index) {
  nlohmann::json json_children;
  try {
    json_children = nlohmann::json::parse(text);
  } catch (const nlohmann::json::parse_error &parse_error) {
    Q_ASSERT(false);
    return;
  }

  auto parent_level = get_level(parent_index);

  static const nlohmann::json_schema::json_validator chords_validator(
      nlohmann::json({
          {"$schema", "http://json-schema.org/draft-07/schema#"},
          {"title", "Chords"},
          {"description", "a list of chords"},
          {"type", "array"},
          {"items", get_chord_schema()},
      }));
  static const nlohmann::json_schema::json_validator notes_validator(
      nlohmann::json({{"$schema", "http://json-schema.org/draft-07/schema#"},
                      {"type", "array"},
                      {"title", "Notes"},
                      {"description", "the notes"},
                      {"items", get_note_schema()}}));
  JsonErrorHandler error_handler(parent_pointer);
  if (parent_level == root_level) {
    chords_validator.validate(json_children, error_handler);
  } else {
    Q_ASSERT(parent_level == chord_level);
    notes_validator.validate(json_children, error_handler);
  }
  if (!error_handler) {
    Q_ASSERT(undo_stack_pointer != nullptr);
    undo_stack_pointer->push(std::make_unique<InsertRemoveChange>(
                                 this, first_child_number, json_children,
                                 to_parent_number(parent_index), true)
                                 .release());
  }
}

auto ChordsModel::copy_rows_to(size_t first_child_number,
                               size_t number_of_children, int parent_number)
    -> nlohmann::json {
  if (parent_number == -1) {
    // or root
    return objects_to_json(chords, first_child_number, number_of_children);
  }
  // for chord
  Q_ASSERT(0 <= parent_number);
  Q_ASSERT(static_cast<size_t>(parent_number) < chords.size());
  return objects_to_json(chords[parent_number].notes, first_child_number,
                         number_of_children);
}

void ChordsModel::copy_rows(size_t first_child_number,
                            size_t number_of_children, int parent_number) {
  if (parent_number == -1) {
    copy_type = chord_copy;
  } else {
    copy_type = note_copy;
  }
  emit copy_type_changed(copy_type);
  copy_json(
      copy_rows_to(first_child_number, number_of_children, parent_number));
}

[[nodiscard]] auto ChordsModel::get_const_note_chord_pointer(
    int parent_number, size_t child_number) const -> const NoteChord * {
  if (parent_number == -1) {
    Q_ASSERT(child_number < chords.size());
    return &chords[child_number];
  }
  Q_ASSERT(parent_number >= 0);
  Q_ASSERT(static_cast<size_t>(parent_number) < chords.size());
  const auto &chord = chords[parent_number];

  const auto &notes = chord.notes;
  Q_ASSERT(child_number < notes.size());
  return &notes[child_number];
}

auto ChordsModel::get_number_of_children(int parent_number) const -> size_t {
  auto chords_size = chords.size();
  if (parent_number == -1) {
    return chords_size;
  }

  Q_ASSERT(0 <= parent_number);
  Q_ASSERT(static_cast<size_t>(parent_number) < chords.size());
  const auto &chord = chords[parent_number];

  return chord.notes.size();
};

void ChordsModel::copy_cell(CellIndex cell_index) {
  copy_type = get_copy_type(cell_index.note_chord_field);
  emit copy_type_changed(copy_type);
  const auto *note_chord_pointer = get_const_note_chord_pointer(
      cell_index.parent_number, cell_index.child_number);
  nlohmann::json copied;
  switch (cell_index.note_chord_field) {
    case symbol_column: {
      Q_ASSERT(false);
      break;
    };
    case instrument_column: {
      copied = nlohmann::json(
          note_chord_pointer->instrument_pointer->instrument_name);
      break;
    }
    case interval_column: {
      copied = note_chord_pointer->interval.json();
      break;
    };
    case beats_column: {
      copied = note_chord_pointer->beats.json();
      break;
    };
    case volume_ratio_column: {
      copied = note_chord_pointer->volume_ratio.json();
      break;
    };
    case tempo_ratio_column: {
      copied = note_chord_pointer->tempo_ratio.json();
      break;
    };
    case words_column: {
      copied = nlohmann::json(note_chord_pointer->words);
      break;
    };
  }
  copy_json(copied);
}

auto get_level(QModelIndex index) -> TreeLevel {
  // root will be an invalid index
  return !index.isValid() ? root_level
         // chords have null parent pointers
         : index.internalPointer() == nullptr ? chord_level
                                              : note_level;
}
