#include "models/ChordsModel.hpp"

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItem...
#include <qassert.h>             // for Q_ASSERT
#include <qcolor.h>              // for QColor
#include <qflags.h>              // for QFlags
#include <qnamespace.h>          // for EditRole, DisplayRole, For...
#include <qstring.h>             // for QString
#include <qtmetamacros.h>        // for emit
#include <qundostack.h>          // for QUndoStack
#include <qvariant.h>            // for QVariant

#include <algorithm>                     // for transform
#include <cstddef>                       // for size_t
#include <map>                           // for operator!=, operator==
#include <memory>                        // for unique_ptr, operator!=
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json-schema.hpp>      // for json_validator
#include <nlohmann/json.hpp>             // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>         // for json
#include <string>                        // for string
#include <vector>                        // for vector

#include "cell_editors/sizes.hpp"          // for get_rational_size, get_ins...
#include "changes/CellChange.hpp"          // for CellChange
#include "changes/InsertRemoveChange.hpp"  // for InsertRemoveChange
#include "json/JsonErrorHandler.hpp"       // for JsonErrorHandler
#include "json/json.hpp"                   // for from_json, to_json
#include "json/schemas.hpp"                // for get_chord_schema, get_note...
#include "justly/Chord.hpp"                // for Chord
#include "justly/Instrument.hpp"           // for Instrument
#include "justly/Interval.hpp"             // for Interval
#include "justly/Note.hpp"                 // for Note
#include "justly/NoteChord.hpp"            // for NoteChord
#include "justly/NoteChordField.hpp"       // for symbol_column, beats_column
#include "justly/Rational.hpp"             // for Rational
#include "justly/Song.hpp"                 // for Song
#include "justly/public_constants.hpp"     // for NON_DEFAULT_COLOR, NOTE_CH...
#include "other/CellIndex.hpp"             // for CellIndex

class QObject;  // lines 19-19

auto text_color(bool is_default) -> QColor {
  return is_default ? DEFAULT_COLOR : NON_DEFAULT_COLOR;
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

  Q_ASSERT(song_pointer != nullptr);
  const auto &chord_pointers = song_pointer->chord_pointers;

  Q_ASSERT(0 <= parent_number);
  Q_ASSERT(static_cast<size_t>(parent_number) < chord_pointers.size());

  return createIndex(int_child_number, note_chord_field,
                     chord_pointers[parent_number].get());
}

auto ChordsModel::get_note_chord_pointer(int parent_number,
                                         size_t child_number) const
    -> NoteChord * {
  Q_ASSERT(song_pointer != nullptr);
  const auto &chord_pointers = song_pointer->chord_pointers;
  if (parent_number == -1) {
    Q_ASSERT(child_number < chord_pointers.size());
    return chord_pointers[child_number].get();
  }

  Q_ASSERT(parent_number >= 0);
  Q_ASSERT(static_cast<size_t>(parent_number) < chord_pointers.size());
  const auto &chord_pointer = chord_pointers[parent_number];
  Q_ASSERT(chord_pointer != nullptr);

  const auto &note_pointers = chord_pointer->note_pointers;
  Q_ASSERT(child_number < note_pointers.size());
  return note_pointers[child_number].get();
};

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

ChordsModel::ChordsModel(Song *song_pointer_input,
                         QUndoStack *undo_stack_pointer_input,
                         QObject *parent_pointer_input)
    : QAbstractItemModel(parent_pointer_input),
      song_pointer(song_pointer_input),
      undo_stack_pointer(undo_stack_pointer_input) {}

auto ChordsModel::copy(size_t first_child_number, size_t number_of_children,
                       int parent_number) const -> nlohmann::json {
  Q_ASSERT(song_pointer != nullptr);
  const auto &chord_pointers = song_pointer->chord_pointers;
  if (parent_number == -1) {
    // or root
    return to_json(chord_pointers, first_child_number, number_of_children);
  }
  // for chord
  Q_ASSERT(0 <= parent_number);
  Q_ASSERT(static_cast<size_t>(parent_number) < chord_pointers.size());
  return to_json(chord_pointers[parent_number]->note_pointers,
                 first_child_number, number_of_children);
}

void ChordsModel::load_chords(const nlohmann::json &json_song) {
  Q_ASSERT(song_pointer != nullptr);
  beginResetModel();
  song_pointer->load_chords(json_song);
  endResetModel();
};

auto ChordsModel::rowCount(const QModelIndex &parent_index) const -> int {
  Q_ASSERT(song_pointer != nullptr);
  const auto &chord_pointers = song_pointer->chord_pointers;
  auto parent_level = get_level(parent_index);
  if (parent_level == root_level) {
    return static_cast<int>(chord_pointers.size());
  }
  if (parent_level == chord_level) {
    if (parent_index.column() != symbol_column) {
      return 0;
    }
    auto chord_number = parent_index.column();
    Q_ASSERT(0 <= chord_number);
    Q_ASSERT(static_cast<size_t>(chord_number) < chord_pointers.size());
    const auto &chord_pointer = chord_pointers[chord_number];
    Q_ASSERT(chord_pointer != nullptr);
    return static_cast<int>(chord_pointer->note_pointers.size());
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
  switch (level) {
    case root_level: {
      Q_ASSERT(false);
      return {};
    }
    case chord_level: {
      return {};
    }
    case note_level: {
      Q_ASSERT(song_pointer != nullptr);
      return createIndex(song_pointer->get_chord_number(
                             static_cast<Chord *>(index.internalPointer())),
                         symbol_column, nullptr);
    }
  }
}

// get a child index
auto ChordsModel::index(int child_number, int column,
                        const QModelIndex &parent_index) const -> QModelIndex {
  return get_index(to_parent_index(parent_index), child_number,
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

  auto *note_chord_pointer =
      get_note_chord_pointer(cell_index.parent_number, cell_index.child_number);

  const auto &instrument_pointer = note_chord_pointer->instrument_pointer;
  Q_ASSERT(instrument_pointer != nullptr);

  switch (cell_index.note_chord_field) {
    case symbol_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString::fromStdString(note_chord_pointer->symbol());
        case Qt::ForegroundRole:
          return NON_DEFAULT_COLOR;
        default:
          return {};
      }
      break;
    case interval_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString::fromStdString(note_chord_pointer->interval.text());
        case Qt::EditRole:
          return QVariant::fromValue(note_chord_pointer->interval);
        case Qt::ForegroundRole:
          return text_color(note_chord_pointer->interval.is_default());
        case Qt::SizeHintRole:
          return get_interval_size();
        default:
          return {};
      }
      break;
    case (beats_column):
      switch (role) {
        case Qt::DisplayRole:
          return QString::fromStdString(note_chord_pointer->beats.text());
        case Qt::ForegroundRole:
          return text_color(note_chord_pointer->beats.is_default());
        case Qt::EditRole:
          return QVariant::fromValue(note_chord_pointer->beats);
        case Qt::SizeHintRole:
          return get_rational_size();
        default:
          return {};
      }
      break;
    case volume_ratio_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString::fromStdString(
              note_chord_pointer->volume_ratio.text());
        case Qt::EditRole:
          return QVariant::fromValue(note_chord_pointer->volume_ratio);
        case Qt::ForegroundRole:
          return text_color(note_chord_pointer->volume_ratio.is_default());
        case Qt::SizeHintRole:
          return get_rational_size();
        default:
          return {};
      }
      break;
    case tempo_ratio_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString::fromStdString(note_chord_pointer->tempo_ratio.text());
        case Qt::EditRole:
          return QVariant::fromValue(note_chord_pointer->tempo_ratio);
        case Qt::ForegroundRole:
          return text_color(note_chord_pointer->tempo_ratio.is_default());
        case Qt::SizeHintRole:
          return get_rational_size();
        default:
          return {};
      }
      break;
    case words_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString::fromStdString(note_chord_pointer->words);
        case Qt::ForegroundRole:
          return text_color(note_chord_pointer->words.empty());
        case Qt::EditRole:
          return QString::fromStdString(note_chord_pointer->words);
        case Qt::SizeHintRole:
          return get_words_size();
        default:
          return {};
      }
      break;
    case instrument_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString::fromStdString(instrument_pointer->instrument_name);
        case Qt::EditRole:
          return QVariant::fromValue(instrument_pointer);
        case Qt::ForegroundRole:
          return text_color(!instrument_pointer->is_default());
        case Qt::SizeHintRole:
          return get_instrument_size();
        default:
          return {};
      }
  }
}

auto ChordsModel::insertRows(int first_child_number, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  auto parent_number = to_parent_index(parent_index);
  nlohmann::json json_objects = nlohmann::json::array();

  Q_ASSERT(song_pointer != nullptr);
  const auto &chord_pointers = song_pointer->chord_pointers;
  if (parent_number == -1) {
    Chord template_chord;
    if (first_child_number > 0) {
      auto previous_number = first_child_number - 1;
      Q_ASSERT(0 <= previous_number);
      Q_ASSERT(static_cast<size_t>(previous_number) <= chord_pointers.size());
      template_chord.beats = chord_pointers[previous_number]->beats;
    }
    for (auto index = 0; index < number_of_children; index = index + 1) {
      json_objects.emplace_back(template_chord.json());
    }
  } else {
    Q_ASSERT(0 <= parent_number);
    Q_ASSERT(static_cast<size_t>(parent_number) <= chord_pointers.size());
    const auto &parent_chord_pointer = chord_pointers[parent_number];
    Q_ASSERT(parent_chord_pointer != nullptr);

    Note template_note;
    if (first_child_number == 0) {
      template_note.beats = parent_chord_pointer->beats;
      template_note.words = parent_chord_pointer->words;
    } else {
      const auto &note_pointers = parent_chord_pointer->note_pointers;
      auto previous_note_number = first_child_number - 1;

      Q_ASSERT(0 <= previous_note_number);
      Q_ASSERT(static_cast<size_t>(previous_note_number) <
               note_pointers.size());
      const auto &previous_note_pointer = note_pointers[previous_note_number];
      Q_ASSERT(previous_note_pointer != nullptr);

      template_note.beats = previous_note_pointer->beats;
      template_note.volume_ratio = previous_note_pointer->volume_ratio;
      template_note.tempo_ratio = previous_note_pointer->tempo_ratio;
      template_note.words = previous_note_pointer->words;
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
  auto parent_number = to_parent_index(parent_index);
  undo_stack_pointer->push(
      std::make_unique<InsertRemoveChange>(
          this, first_child_number,
          copy(first_child_number, number_of_children, parent_number),
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

void ChordsModel::insert_directly(size_t first_child_number,
                         const nlohmann::json &json_children,
                         int parent_number) {
  auto &chord_pointers = song_pointer->chord_pointers;
  beginInsertRows(
      make_chord_index(parent_number), static_cast<int>(first_child_number),
      static_cast<int>(first_child_number + json_children.size()) - 1);
  if (parent_number == -1) {
    from_json(&chord_pointers, first_child_number, json_children);
  } else {
    Q_ASSERT(0 <= parent_number);
    Q_ASSERT(static_cast<size_t>(parent_number) < chord_pointers.size());
    // for a chord
    from_json(&chord_pointers[parent_number]->note_pointers, first_child_number,
              json_children);
  }
  endInsertRows();
}

void ChordsModel::remove_directly(size_t first_child_number, size_t number_of_children,
                         int parent_number) {
  auto &chord_pointers = song_pointer->chord_pointers;

  auto int_first_child_number = static_cast<int>(first_child_number);

  auto end_number = first_child_number + number_of_children;
  auto int_end_number = static_cast<int>(end_number);

  beginRemoveRows(make_chord_index(parent_number), int_first_child_number,
                  int_end_number - 1);
  if (parent_number == -1) {
    // for root
    Q_ASSERT(first_child_number < chord_pointers.size());
    Q_ASSERT(end_number <= chord_pointers.size());
    chord_pointers.erase(chord_pointers.begin() + int_first_child_number,
                         chord_pointers.begin() + int_end_number);
  } else {
    // for a chord
    Q_ASSERT(0 <= parent_number);
    Q_ASSERT(static_cast<size_t>(parent_number) < chord_pointers.size());
    const auto &chord_pointer = chord_pointers[parent_number];
    Q_ASSERT(chord_pointer != nullptr);

    auto &note_pointers = chord_pointer->note_pointers;

    Q_ASSERT(first_child_number < note_pointers.size());

    Q_ASSERT(0 < end_number);
    Q_ASSERT(end_number <= note_pointers.size());

    note_pointers.erase(note_pointers.begin() + int_first_child_number,
                        note_pointers.begin() + int_end_number);
  }
  endRemoveRows();
}

void ChordsModel::set_cell_directly(const CellIndex &cell_index,
                           const QVariant &new_value) {
  auto parent_number = cell_index.parent_number;
  auto child_number = cell_index.child_number;
  auto note_chord_field = cell_index.note_chord_field;

  auto *note_chord_pointer =
      get_note_chord_pointer(cell_index.parent_number, cell_index.child_number);

  const auto &instrument_pointer = note_chord_pointer->instrument_pointer;
  Q_ASSERT(instrument_pointer != nullptr);

  switch (note_chord_field) {
    case symbol_column:
      break;
    case interval_column:
      Q_ASSERT(new_value.canConvert<Interval>());
      note_chord_pointer->interval = new_value.value<Interval>();
      break;
    case beats_column:
      Q_ASSERT(new_value.canConvert<Rational>());
      note_chord_pointer->beats = new_value.value<Rational>();
      break;
    case volume_ratio_column:
      Q_ASSERT(new_value.canConvert<Rational>());
      note_chord_pointer->volume_ratio = new_value.value<Rational>();
      break;
    case tempo_ratio_column:
      Q_ASSERT(new_value.canConvert<Rational>());
      note_chord_pointer->tempo_ratio = new_value.value<Rational>();
      break;
    case words_column:
      Q_ASSERT(new_value.canConvert<QString>());
      note_chord_pointer->words = new_value.toString().toStdString();
      break;
    case instrument_column:
      Q_ASSERT(new_value.canConvert<const Instrument *>());
      note_chord_pointer->instrument_pointer =
          new_value.value<const Instrument *>();
      break;
  }
  auto index = get_index(parent_number, child_number, note_chord_field);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, Qt::EditRole});
}

auto get_level(QModelIndex index) -> TreeLevel {
  // root will be an invalid index
  return !index.isValid() ? root_level
         // chords have null parent pointers
         : index.internalPointer() == nullptr ? chord_level
                                              : note_level;
}

auto verify_children(const QModelIndex &parent_index,
                     const nlohmann::json &json_children) -> bool {
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
  JsonErrorHandler error_handler;
  auto parent_level = get_level(parent_index);

  if (parent_level == root_level) {
    chords_validator.validate(json_children, error_handler);
  } else {
    Q_ASSERT(parent_level == chord_level);
    notes_validator.validate(json_children, error_handler);
  }
  return !error_handler;
}

auto to_parent_index(const QModelIndex &index) -> int {
  auto level = get_level(index);
  if (level == root_level) {
    return -1;
  }
  Q_ASSERT(level == chord_level);
  return index.row();
}