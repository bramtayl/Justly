#include "models/ChordsModel.hpp"

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItem...
#include <qcolor.h>              // for QColor
#include <qflags.h>              // for QFlags
#include <qnamespace.h>          // for EditRole, DisplayRole, For...
#include <qstring.h>             // for QString
#include <qtmetamacros.h>        // for emit
#include <qundostack.h>          // for QUndoStack
#include <qvariant.h>            // for QVariant

#include <algorithm>                     // for transform, find_if
#include <cstddef>                       // for size_t
#include <map>                           // for operator!=, operator==
#include <memory>                        // for unique_ptr, make_unique
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json-schema.hpp>      // for json_validator
#include <nlohmann/json.hpp>             // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>         // for json
#include <string>                        // for string
#include <vector>                        // for vector

#include "changes/CellChange.hpp"          // for CellChange
#include "changes/InsertRemoveChange.hpp"  // for InsertRemoveChange
#include "editors/sizes.hpp"               // for get_rational_size, get_ins...
#include "json/JsonErrorHandler.hpp"       // for JsonErrorHandler
#include "json/schemas.hpp"                // for get_chord_schema, get_note...
#include "justly/Chord.hpp"                // for Chord
#include "justly/Instrument.hpp"           // for get_instrument, Instrument
#include "justly/Interval.hpp"             // for Interval
#include "justly/Note.hpp"                 // for Note
#include "justly/NoteChord.hpp"            // for NoteChord
#include "justly/NoteChordField.hpp"       // for symbol_column, beats_column
#include "justly/Rational.hpp"             // for Rational
#include "justly/Song.hpp"                 // for Song
#include "justly/public_constants.hpp"     // for NON_DEFAULT_COLOR, DEFAULT...
#include "song/SongIndex.hpp"              // for SongIndex
#include "song/json.hpp"                   // for from_json, to_json

class QObject;  // lines 19-19

auto text_color(bool is_default) -> QColor {
  return is_default ? DEFAULT_COLOR : NON_DEFAULT_COLOR;
}

auto ChordsModel::make_chord_index(int parent_number) const -> QModelIndex {
  // for root, use an empty index
  return parent_number == -1
             ? QModelIndex()
             // for chords, the parent pointer is null
             : createIndex(parent_number, symbol_column, nullptr);
}

auto ChordsModel::get_index(int chord_number, int note_number,
                            NoteChordField column_number) const -> QModelIndex {
  auto root_index = QModelIndex();
  if (chord_number == -1) {
    return root_index;
  }
  if (note_number == -1) {
    return index(chord_number, column_number, root_index);
  }
  return index(note_number, column_number,
               index(chord_number, symbol_column, root_index));
}

auto ChordsModel::to_song_index(const QModelIndex &index) const -> SongIndex {
  auto level = get_level(index);
  return SongIndex(
      // for notes, the row is the note number, otherwise, there is no note
      // number
      {get_parent_number(index), level == note_level ? index.row() : -1,
       // for the root, the field is always the symbol column
       level == root_level ? symbol_column : index.column()});
}

ChordsModel::ChordsModel(Song *song_pointer_input,
                         QUndoStack *undo_stack_pointer_input,
                         QObject *parent_pointer_input)
    : QAbstractItemModel(parent_pointer_input),
      song_pointer(song_pointer_input),
      undo_stack_pointer(undo_stack_pointer_input) {}

auto ChordsModel::get_parent_number(const QModelIndex &index) const -> int {
  auto *chord_pointer = index.internalPointer();

  Q_ASSERT(song_pointer != nullptr);

  auto &chord_pointers = song_pointer->chord_pointers;
  switch (get_level(index)) {
    case root_level:
      return -1;
    case chord_level:
      return index.row();
    case note_level:
      for (size_t note_index = 0; note_index < chord_pointers.size();
           note_index = note_index + 1) {
        if (chord_pointer == chord_pointers[note_index].get()) {
          return static_cast<int>(note_index);
        }
      }
      Q_ASSERT(false);
      return 0;
    default:
      Q_ASSERT(false);
      return {};
  }
}

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
  auto parent_level = get_level(parent_index);
  // for root, we dont care about the column
  size_t result = 0;
  Q_ASSERT(song_pointer != nullptr);
  const auto& chord_pointers = song_pointer->chord_pointers;
  if (parent_level == root_level) {
    result = chord_pointers.size();
  } else if (parent_index.column() == symbol_column &&
             parent_level == chord_level) {
    auto parent_number = get_parent_number(parent_index);
    Q_ASSERT(0 <= parent_number);
    Q_ASSERT(static_cast<size_t>(parent_number) < chord_pointers.size());
    result = chord_pointers[parent_number]
                 ->note_pointers.size();
  }
  return static_cast<int>(result);
}

auto ChordsModel::columnCount(const QModelIndex & /*parent*/) const -> int {
  return NOTE_CHORD_COLUMNS;
}

// get the parent index
auto ChordsModel::parent(const QModelIndex &index) const -> QModelIndex {
  return get_level(index) == note_level
             // for notes, the parent is a chord, which has a parent of null
             ? createIndex(get_parent_number(index), symbol_column, nullptr)
             // for chords, the parent is root
             : QModelIndex();
}

// get a child index
auto ChordsModel::index(int child_number, int note_chord_field,
                        const QModelIndex &parent_index) const -> QModelIndex {
  // will error if row doesn't exist
  if (!parent_index.isValid()) {
    // for root, the child will be a chord, with a null parent parent
    return createIndex(child_number, note_chord_field, nullptr);
  }
  auto parent_number = get_parent_number(parent_index);
  Q_ASSERT(song_pointer != nullptr);
  const auto &chord_pointers = song_pointer->chord_pointers;
  Q_ASSERT(0 <= parent_number);
  Q_ASSERT(static_cast<size_t>(parent_number) < chord_pointers.size());
  return createIndex(child_number, note_chord_field,
                     chord_pointers[parent_number].get());
}

auto ChordsModel::headerData(int section, Qt::Orientation orientation,
                             int role) const -> QVariant {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch (section) {
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
      default:
        Q_ASSERT(false);
        return {};
    }
  }
  // no horizontal headers
  // no headers for other roles
  return {};
}

auto ChordsModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  auto note_chord_field = index.column();
  auto selectable = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  if (note_chord_field == symbol_column) {
    return selectable;
  }
  return selectable | Qt::ItemIsEditable;
}

auto ChordsModel::data(const QModelIndex &index, int role) const -> QVariant {
  auto song_index = to_song_index(index);

  Q_ASSERT(song_pointer != nullptr);
  const auto &chord_pointers = song_pointer->chord_pointers;

  auto chord_number = song_index.chord_number;
  Q_ASSERT(0 <= chord_number);
  Q_ASSERT(static_cast<size_t>(chord_number) < chord_pointers.size());
  const auto &chord_pointer = chord_pointers[chord_number];

  auto note_number = song_index.note_number;
  auto note_chord_field = song_index.note_chord_field;

  NoteChord *note_chord_pointer = nullptr;
  if (note_number == -1) {
    note_chord_pointer = chord_pointer.get();
  } else {
    Q_ASSERT(chord_pointer != nullptr);
    const auto &note_pointers = chord_pointer->note_pointers;

    Q_ASSERT(0 <= note_number);
    Q_ASSERT(static_cast<size_t>(note_number) < note_pointers.size());
    note_chord_pointer = note_pointers[note_number].get();
  }

  Q_ASSERT(note_chord_pointer != nullptr);

  const auto &instrument_pointer = note_chord_pointer->instrument_pointer;
  Q_ASSERT(instrument_pointer != nullptr);

  switch (note_chord_field) {
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
    default:
      Q_ASSERT(false);
      return {};
  }
}

auto ChordsModel::insertRows(int first_child_number, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  auto parent_number = get_parent_number(parent_index);
  nlohmann::json json_objects = nlohmann::json::array();

  Q_ASSERT(song_pointer != nullptr);
  const auto &chord_pointers = song_pointer->chord_pointers;
  if (parent_number == -1) {
    Chord template_chord;
    if (first_child_number > 0) {
      auto previous_number = first_child_number - 1;
      Q_ASSERT(0 <= previous_number);
      Q_ASSERT(static_cast<size_t>(previous_number) <= chord_pointers.size());
      template_chord.beats = chord_pointers[first_child_number - 1]->beats;
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
      Q_ASSERT(static_cast<size_t>(previous_note_number) < note_pointers.size());
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
  auto parent_number = get_parent_number(parent_index);
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
      std::make_unique<CellChange>(this, to_song_index(index),
                                   data(index, Qt::EditRole), new_value)
          .release());
  return true;
}

void ChordsModel::insert(int first_child_number,
                         const nlohmann::json &json_children,
                         int parent_number) {
  auto &chord_pointers = song_pointer->chord_pointers;
  beginInsertRows(
      make_chord_index(parent_number), first_child_number,
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

void ChordsModel::remove(int first_child_number, int number_of_children,
                         int parent_number) {
  auto &chord_pointers = song_pointer->chord_pointers;
  auto end_number = first_child_number + number_of_children;

  beginRemoveRows(make_chord_index(parent_number), first_child_number,
                  end_number - 1);
  if (parent_number == -1) {
    // for root
    Q_ASSERT(0 <= first_child_number);
    Q_ASSERT(static_cast<size_t>(first_child_number) < chord_pointers.size());

    Q_ASSERT(0 < end_number);
    Q_ASSERT(static_cast<size_t>(end_number) <= chord_pointers.size());
    chord_pointers.erase(chord_pointers.begin() + first_child_number,
                         chord_pointers.begin() + end_number);
  } else {
    // for a chord
    Q_ASSERT(0 <= parent_number);
    Q_ASSERT(static_cast<size_t>(parent_number) < chord_pointers.size());
    const auto &chord_pointer = chord_pointers[parent_number];
    Q_ASSERT(chord_pointer != nullptr);

    auto &note_pointers = chord_pointer->note_pointers;

    Q_ASSERT(0 <= first_child_number);
    Q_ASSERT(static_cast<size_t>(first_child_number) < note_pointers.size());

    Q_ASSERT(0 < end_number);
    Q_ASSERT(static_cast<size_t>(end_number) <= note_pointers.size());

    note_pointers.erase(note_pointers.begin() + first_child_number,
                        note_pointers.begin() + end_number);
  }
  endRemoveRows();
}

void ChordsModel::set_cell(const SongIndex &song_index,
                           const QVariant &new_value) {
  auto chord_number = song_index.chord_number;

  Q_ASSERT(song_pointer != nullptr);
  auto &chord_pointers = song_pointer->chord_pointers;

  Q_ASSERT(0 <= chord_number);
  Q_ASSERT(static_cast<size_t>(chord_number) < chord_pointers.size());
  auto &chord_pointer = chord_pointers[chord_number];
  Q_ASSERT(chord_pointer != nullptr);

  auto note_number = song_index.note_number;

  NoteChord *note_chord_pointer = nullptr;
  if (note_number == -1) {
    note_chord_pointer = chord_pointer.get();
  } else {
    auto &note_pointers = chord_pointer->note_pointers;
    Q_ASSERT(0 <= note_number);
    Q_ASSERT(static_cast<size_t>(note_number) <= note_pointers.size());
    note_chord_pointer = note_pointers[note_number].get();
  }
  Q_ASSERT(note_chord_pointer != nullptr);

  auto note_chord_field = song_index.note_chord_field;
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
      note_chord_pointer->instrument_pointer = new_value.value<const Instrument *>();
      break;
    default:
      break;
  }
  auto index =
      // it's root, so return an invalid index
      chord_number == -1 ? QModelIndex()
      : note_number == -1
          // for chords, the row is the chord number, and the parent
          // pointer is null
          ? createIndex(chord_number, note_chord_field, nullptr)
          // for notes, the row is the note number, and the parent pointer
          // is the chord pointer
          : createIndex(note_number, note_chord_field, chord_pointer.get());
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, Qt::EditRole});
}

auto get_level(QModelIndex index) -> TreeLevel {
  // root will be an invalid index
  return index.row() == -1 ? root_level
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
  if (get_level(parent_index) == root_level) {
    chords_validator.validate(json_children, error_handler);
  } else {
    notes_validator.validate(json_children, error_handler);
  }
  return !error_handler;
}
