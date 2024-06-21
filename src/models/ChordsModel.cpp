#include "models/ChordsModel.hpp"

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItem...
#include <qcolor.h>              // for QColor
#include <qflags.h>              // for QFlags
#include <qnamespace.h>          // for EditRole, DisplayRole, For...
#include <qstring.h>             // for QString
#include <qtmetamacros.h>        // for emit
#include <qundostack.h>          // for QUndoStack
#include <qvariant.h>            // for QVariant
#include <qwidget.h>             // for QWidget

#include <algorithm>                     // for transform, max, find_if
#include <cstddef>                       // for size_t
#include <map>                           // for operator!=, operator==
#include <memory>                        // for unique_ptr, make_unique
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json-schema.hpp>      // for json_validator
#include <nlohmann/json.hpp>             // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>         // for json
#include <string>                        // for operator==, char_traits
#include <vector>                        // for vector

#include "changes/CellChange.hpp"          // for CellChange
#include "changes/InsertRemoveChange.hpp"  // for InsertRemoveChange
#include "editors/ChordsDelegate.hpp"      // for create_editor
#include "json/JsonErrorHandler.hpp"       // for JsonErrorHandler
#include "json/schemas.hpp"                // for get_chord_schema, get_note...
#include "justly/Chord.hpp"                // for Chord
#include "justly/Instrument.hpp"           // for get_instrument, Instrument
#include "justly/Interval.hpp"             // for Interval
#include "justly/Note.hpp"                 // for Note
#include "justly/NoteChord.hpp"            // for NoteChord, DEFAULT_BEATS
#include "justly/NoteChordField.hpp"       // for symbol_column, beats_column
#include "justly/Song.hpp"                 // for Song
#include "justly/SongIndex.hpp"            // for SongIndex
#include "justly/constants.hpp"            // for NON_DEFAULT_COLOR, DEFAULT...
#include "song/json.hpp"                   // for from_json, insert_objects

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
  auto &chord_pointers = song_pointer->chord_pointers;
  switch (get_level(index)) {
    case root_level:
      return -1;
    case chord_level:
      return index.row();
    case note_level:
      return static_cast<int>(
          std::find_if(
              chord_pointers.begin(), chord_pointers.end(),
              [chord_pointer](std::unique_ptr<Chord> &maybe_chord_pointer) {
                return maybe_chord_pointer.get() == chord_pointer;
              }) -
          chord_pointers.begin());
    default:
      return {};
  }
}

auto ChordsModel::copy(size_t first_child_number, size_t number_of_children,
                       int parent_number) const -> nlohmann::json {
  return parent_number == -1
             // for root
             ? to_json(song_pointer->chord_pointers, first_child_number,
                       number_of_children)
             // for a chord
             : to_json(
                   song_pointer->chord_pointers[parent_number]->note_pointers,
                   first_child_number, number_of_children);
}

void ChordsModel::load_chords(const nlohmann::json &json_song) {
  beginResetModel();
  song_pointer->load_chords(json_song);
  endResetModel();
};

auto ChordsModel::rowCount(const QModelIndex &parent_index) const -> int {
  auto parent_level = get_level(parent_index);
  // for root, we dont care about the column
  size_t result = 0;
  if (parent_level == root_level) {
    result = song_pointer->chord_pointers.size();
  } else if (parent_index.column() == symbol_column &&
             parent_level == chord_level) {
    result = song_pointer->chord_pointers[get_parent_number(parent_index)]
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
  // createIndex needs a pointer to the item, not the parent
  // will error if row doesn't exist
  return createIndex(
      child_number, note_chord_field,
      // for root, the child will be a chord, with a null parent parent
      get_level(parent_index) == root_level
          ? nullptr
          // for chords, the child will be a note, with a chord parent pointer
          : song_pointer->chord_pointers[get_parent_number(parent_index)]
                .get());
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
      case volume_percent_column:
        return tr("Volume");
      case tempo_percent_column:
        return tr("Tempo");
      case words_column:
        return tr("Words");
      case instrument_column:
        return tr("Instrument");
      default:
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
  auto &chord_pointer = song_pointer->chord_pointers[song_index.chord_number];
  auto note_number = song_index.note_number;
  auto note_chord_field = song_index.note_chord_field;

  NoteChord *note_chord_pointer = nullptr;
  if (note_number == -1) {
    note_chord_pointer = chord_pointer.get();
  } else {
    note_chord_pointer = chord_pointer->note_pointers[note_number].get();
  }

  static auto interval_cell_size =
      create_editor(nullptr, interval_column)->sizeHint();
  static auto beats_cell_size =
      create_editor(nullptr, beats_column)->sizeHint();
  static auto instrument_cell_size =
      create_editor(nullptr, instrument_column)->sizeHint();
  static auto tempo_percent_cell_size =
      create_editor(nullptr, tempo_percent_column)->sizeHint();
  static auto volume_percent_cell_size =
      create_editor(nullptr, volume_percent_column)->sizeHint();
  static auto words_cell_size =
      create_editor(nullptr, words_column)->sizeHint();

  switch (note_chord_field) {
    case symbol_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString::fromStdString(note_chord_pointer->symbol());
        case Qt::ForegroundRole:
          return NON_DEFAULT_COLOR;
        default:
          break;
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
          return interval_cell_size;
        default:
          break;
      }
      break;
    case (beats_column):
      switch (role) {
        case Qt::DisplayRole:
          return note_chord_pointer->beats;
        case Qt::ForegroundRole:
          return text_color(note_chord_pointer->beats == DEFAULT_BEATS);
        case Qt::EditRole:
          return note_chord_pointer->beats;
        case Qt::SizeHintRole:
          return beats_cell_size;
        default:
          break;
      }
      break;
    case volume_percent_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString("%1%").arg(note_chord_pointer->volume_percent);
        case Qt::EditRole:
          return note_chord_pointer->volume_percent;
        case Qt::ForegroundRole:
          return text_color(note_chord_pointer->volume_percent ==
                            DEFAULT_VOLUME_PERCENT);
        case Qt::SizeHintRole:
          return volume_percent_cell_size;
        default:
          break;
      }
      break;
    case tempo_percent_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString("%1%").arg(note_chord_pointer->tempo_percent);
        case Qt::EditRole:
          return note_chord_pointer->tempo_percent;
        case Qt::ForegroundRole:
          return text_color(note_chord_pointer->tempo_percent ==
                            DEFAULT_TEMPO_PERCENT);
        case Qt::SizeHintRole:
          return tempo_percent_cell_size;
        default:
          break;
      }
      break;
    case words_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString::fromStdString(note_chord_pointer->words);
        case Qt::ForegroundRole:
          return text_color(note_chord_pointer->words == DEFAULT_WORDS);
        case Qt::EditRole:
          return QString::fromStdString(note_chord_pointer->words);
        case Qt::SizeHintRole:
          return words_cell_size;
        default:
          break;
      }
      break;
    case instrument_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString::fromStdString(
              note_chord_pointer->instrument_pointer->instrument_name);
        case Qt::EditRole:
          return QVariant::fromValue(note_chord_pointer->instrument_pointer);
        case Qt::ForegroundRole:
          return text_color(note_chord_pointer->instrument_pointer ==
                            &get_instrument(""));
        case Qt::SizeHintRole:
          return instrument_cell_size;
        default:
          break;
      }
    default:
      break;
  }
  // no data for other roles
  return {};
}

auto ChordsModel::insertRows(int first_child_number, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  auto parent_number = get_parent_number(parent_index);
  nlohmann::json json_objects = nlohmann::json::array();
  if (parent_number == -1) {
    Chord template_chord;
    if (first_child_number > 0) {
      template_chord.beats =
          song_pointer->chord_pointers[first_child_number - 1]->beats;
    }
    for (auto index = 0; index < number_of_children; index = index + 1) {
      json_objects.emplace_back(template_chord.json());
    }
  } else {
    auto &parent_chord_pointer = song_pointer->chord_pointers[parent_number];
    Note template_note;
    if (first_child_number == 0) {
      template_note.beats = parent_chord_pointer->beats;
      template_note.words = parent_chord_pointer->words;
    } else {
      auto &previous_note_pointer =
          parent_chord_pointer->note_pointers[first_child_number - 1];
      template_note.beats = previous_note_pointer->beats;
      template_note.volume_percent = previous_note_pointer->volume_percent;
      template_note.tempo_percent = previous_note_pointer->tempo_percent;
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
  beginInsertRows(
      make_chord_index(parent_number), first_child_number,
      static_cast<int>(first_child_number + json_children.size()) - 1);
  if (parent_number == -1) {
    // for root
    from_json(&song_pointer->chord_pointers, first_child_number, json_children);
  } else {
    // for a chord
    from_json(&song_pointer->chord_pointers[parent_number]->note_pointers,
              first_child_number, json_children);
  }
  endInsertRows();
}

void ChordsModel::remove(int first_child_number, int number_of_children,
                         int parent_number) {
  beginRemoveRows(make_chord_index(parent_number), first_child_number,
                  first_child_number + number_of_children - 1);
  if (parent_number == -1) {
    // for root
    auto &chord_pointers = song_pointer->chord_pointers;
    chord_pointers.erase(
        chord_pointers.begin() + first_child_number,
        chord_pointers.begin() + first_child_number + number_of_children);
  } else {
    // for a chord
    auto &note_pointers =
        song_pointer->chord_pointers[parent_number]->note_pointers;
    note_pointers.erase(
        note_pointers.begin() + first_child_number,
        note_pointers.begin() + first_child_number + number_of_children);
  }
  endRemoveRows();
}

void ChordsModel::set_cell(const SongIndex &song_index,
                           const QVariant &new_value) {
  auto &chord_pointer = song_pointer->chord_pointers[song_index.chord_number];
  auto chord_number = song_index.chord_number;
  auto note_number = song_index.note_number;
  auto note_chord_field = song_index.note_chord_field;
  NoteChord *note_chord_pointer = nullptr;
  if (note_number == -1) {
    note_chord_pointer = chord_pointer.get();
  } else {
    note_chord_pointer = chord_pointer->note_pointers[note_number].get();
  }
  switch (note_chord_field) {
    case symbol_column:
      break;
    case interval_column:
      note_chord_pointer->interval = new_value.value<Interval>();
      break;
    case beats_column:
      note_chord_pointer->beats = new_value.toInt();
      break;
    case volume_percent_column:
      note_chord_pointer->volume_percent = new_value.toDouble();
      break;
    case tempo_percent_column:
      note_chord_pointer->tempo_percent = new_value.toDouble();
      break;
    case words_column:
      note_chord_pointer->words = new_value.toString().toStdString();
      break;
    case instrument_column:
      note_chord_pointer->instrument_pointer =
          new_value.value<const Instrument *>();
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
          : createIndex(note_number, note_chord_field,
                        song_pointer->chord_pointers[chord_number].get());
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
