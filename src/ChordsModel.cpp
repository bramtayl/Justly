#include "src/ChordsModel.h"

#include <QtCore/qglobal.h>      // for QFlags<>::enum_type, QFlags
#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemM...
#include <qnamespace.h>          // for EditRole, DisplayRole, Fore...
#include <qstring.h>             // for QString
#include <qtmetamacros.h>        // for emit
#include <qundostack.h>          // for QUndoStack
#include <qvariant.h>            // for QVariant
#include <qwidget.h>

#include <algorithm>
#include <map>                           // for operator!=
#include <memory>                        // for unique_ptr, allocator_trait...
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json-schema.hpp>      // for json_validator
#include <nlohmann/json.hpp>             // for basic_json<>::object_t, bas...
#include <nlohmann/json_fwd.hpp>         // for json
#include <string>                        // for operator==, string, basic_s...
#include <vector>                        // for vector

#include "justly/Chord.h"            // for Chord
#include "justly/Interval.h"         // for Interval
#include "justly/Note.h"             // for Note
#include "justly/NoteChord.h"        // for NoteChord, symbol_column
#include "justly/NoteChordField.h"   // for symbol_column, NoteChordField
#include "justly/Song.h"             // for Song
#include "src/CellChange.h"          // for CellChange
#include "src/ChordsDelegate.h"      // for ChordsDelegate
#include "src/InsertEmptyChange.h"   // for InsertEmptyChange
#include "src/InsertRemoveChange.h"  // for InsertRemoveChange
#include "src/Instrument.h"          // for Instrument
#include "src/JsonErrorHandler.h"    // for JsonErrorHandler
#include "src/SongIndex.h"           // for SongIndex

class QObject;  // lines 19-19

auto ChordsModel::get_tree_index(const SongIndex &song_index) const
    -> QModelIndex {
  auto chord_number = song_index.chord_number;
  if (chord_number == -1) {
    // it's root, so return an invalid index
    return {};
  }
  auto note_chord_field = song_index.note_chord_field;
  auto note_number = song_index.note_number;
  return note_number == -1
             // for chords, the row is the chord number, and the parent pointer
             // is null
             ? createIndex(chord_number, note_chord_field, nullptr)
             // for notes, the row is the note number, and the parent pointer is
             // the chord pointer
             : createIndex(note_number, note_chord_field,
                           song_pointer->chord_pointers[chord_number].get());
};

auto ChordsModel::get_song_index(const QModelIndex &index) const -> SongIndex {
  auto level = ChordsModel::get_level(index);
  return SongIndex(
      // for notes, the row is the note number, otherwise, there is no note
      // number
      {get_chord_number(index), level == note_level ? index.row() : -1,
       // for the root, the field is always the symbol column
       level == root_level ? symbol_column
                           : static_cast<NoteChordField>(index.column())});
}

auto ChordsModel::get_chord_index(int chord_number) const -> QModelIndex {
  // for root, use an empty index
  return chord_number == -1 ? QModelIndex()
                            // for chords, the parent pointer is null
                            : createIndex(chord_number, symbol_column, nullptr);
}

auto ChordsModel::get_chord_number(const QModelIndex &index) const -> int {
  switch (ChordsModel::get_level(index)) {
    case root_level:
      return -1;
    case chord_level:
      return index.row();
    default:
      auto *chord_pointer = index.internalPointer();
      auto &chord_pointers = song_pointer->chord_pointers;
      return static_cast<int>(
          std::find_if(
              chord_pointers.begin(), chord_pointers.end(),
              [chord_pointer](std::unique_ptr<Chord> &maybe_chord_pointer) {
                return maybe_chord_pointer.get() == chord_pointer;
              }) -
          chord_pointers.begin());
  }
}

ChordsModel::ChordsModel(Song *song_pointer_input,
                         QUndoStack *undo_stack_pointer_input,
                         QObject *parent_pointer_input)
    : QAbstractItemModel(parent_pointer_input),
      song_pointer(song_pointer_input),
      undo_stack_pointer(undo_stack_pointer_input) {}

auto ChordsModel::columnCount(const QModelIndex & /*parent*/) const -> int {
  return NOTE_CHORD_COLUMNS;
}

auto ChordsModel::data(const QModelIndex &index, int role) const -> QVariant {
  auto song_index = get_song_index(index);
  auto &chord_pointer = song_pointer->chord_pointers[song_index.chord_number];
  auto note_number = song_index.note_number;
  auto note_chord_field = song_index.note_chord_field;

  auto *note_chord_pointer =
      note_number == -1 ? static_cast<NoteChord *>(chord_pointer.get())
                        // for notes
                        : static_cast<NoteChord *>(
                              chord_pointer->note_pointers[note_number].get());

  static auto interval_cell_size =
      ChordsDelegate::create_editor(nullptr, interval_column)->sizeHint();
  static auto beats_cell_size =
      ChordsDelegate::create_editor(nullptr, beats_column)->sizeHint();
  static auto instrument_cell_size =
      ChordsDelegate::create_editor(nullptr, instrument_column)->sizeHint();
  static auto tempo_percent_cell_size =
      ChordsDelegate::create_editor(nullptr, tempo_percent_column)->sizeHint();
  static auto volume_percent_cell_size =
      ChordsDelegate::create_editor(nullptr, volume_percent_column)->sizeHint();
  static auto words_cell_size =
      ChordsDelegate::create_editor(nullptr, words_column)->sizeHint();

  switch (note_chord_field) {
    case symbol_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString::fromStdString(note_chord_pointer->symbol_for());
        case Qt::ForegroundRole:
          return NON_DEFAULT_COLOR;
        default:
          break;
      }
      break;
    case interval_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString::fromStdString(
              note_chord_pointer->interval.get_text());
        case Qt::EditRole:
          return QVariant::fromValue(note_chord_pointer->interval);
        case Qt::ForegroundRole:
          return get_text_color(note_chord_pointer->interval.is_default());
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
          return get_text_color(note_chord_pointer->beats == DEFAULT_BEATS);
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
          return get_text_color(note_chord_pointer->volume_percent ==
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
          return get_text_color(note_chord_pointer->tempo_percent ==
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
          return get_text_color(note_chord_pointer->words == DEFAULT_WORDS);
        case Qt::EditRole:
          return QString::fromStdString(note_chord_pointer->words);
        case Qt::SizeHintRole:
          return words_cell_size;
        default:
          break;
      }
      break;
    default:  // instrument_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString::fromStdString(
              note_chord_pointer->instrument_pointer->instrument_name);
        case Qt::EditRole:
          return QVariant::fromValue(note_chord_pointer->instrument_pointer);
        case Qt::ForegroundRole:
          return get_text_color(note_chord_pointer->instrument_pointer ==
                                &Instrument::get_empty_instrument());
        case Qt::SizeHintRole:
          return instrument_cell_size;
        default:
          break;
      }
  }
  // no data for other roles
  return {};
}

auto ChordsModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  switch (static_cast<NoteChordField>(index.column())) {
    case symbol_column:
      return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    default:
      return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
  }
}

auto ChordsModel::headerData(int section, Qt::Orientation orientation,
                             int role) const -> QVariant {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch (static_cast<NoteChordField>(section)) {
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
      default:  // symbol_column
        return {};
    }
  }
  // no horizontal headers
  // no headers for other roles
  return {};
}

// get the parent index
auto ChordsModel::parent(const QModelIndex &index) const -> QModelIndex {
  return ChordsModel::get_level(index) == note_level
             // for notes, the parent is a chord, which has a parent of null
             ? createIndex(get_chord_number(index), symbol_column, nullptr)
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
          : song_pointer->chord_pointers[get_chord_number(parent_index)].get());
}

auto ChordsModel::rowCount(const QModelIndex &parent_index) const -> int {
  auto parent_level = get_level(parent_index);
  // for root, we dont care about the column
  return parent_level == root_level
             ? static_cast<int>(song_pointer->chord_pointers.size())
         // for chords, nest into the symbol column
         : parent_index.column() == symbol_column && parent_level == chord_level
             ? static_cast<int>(
                   song_pointer->chord_pointers[get_chord_number(parent_index)]
                       ->note_pointers.size())
             // notes dont have children
             : 0;
}

// node will check for errors, so no need to check for errors here
void ChordsModel::set_data_directly(const SongIndex &song_index,
                                    const QVariant &new_value) {
  auto &chord_pointer = song_pointer->chord_pointers[song_index.chord_number];
  auto note_number = song_index.note_number;
  auto note_chord_field = song_index.note_chord_field;
  auto *note_chord_pointer =
      note_number == -1 ? static_cast<NoteChord *>(chord_pointer.get())
                        : static_cast<NoteChord *>(
                              chord_pointer->note_pointers[note_number].get());
  switch (note_chord_field) {
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
    default:  // symbol_column
      break;
  }
  auto index = get_tree_index(song_index);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, Qt::EditRole});
}

auto ChordsModel::setData(const QModelIndex &index, const QVariant &new_value,
                          int role) -> bool {
  if (role != Qt::EditRole) {
    return false;
  }
  undo_stack_pointer->push(
      std::make_unique<CellChange>(this, get_song_index(index),
                                   data(index, Qt::EditRole), new_value)
          .release());
  return true;
}

void ChordsModel::remove_children_directly(int first_child_number,
                                           int number_of_children,
                                           int chord_number) {
  beginRemoveRows(get_chord_index(chord_number), first_child_number,
                  first_child_number + number_of_children - 1);
  if (chord_number == -1) {
    // for root
    song_pointer->remove_children(first_child_number, number_of_children);
  } else {
    // for a chord
    song_pointer->chord_pointers[chord_number]->remove_children(
        first_child_number, number_of_children);
  }
  endRemoveRows();
}

void ChordsModel::insert_empty_children_directly(int first_child_number,
                                                 int number_of_children,
                                                 int chord_number) {
  beginInsertRows(get_chord_index(chord_number), first_child_number,
                  first_child_number + number_of_children - 1);
  if (chord_number == -1) {
    // for root
    song_pointer->insert_empty_chilren(first_child_number, number_of_children);
  } else {
    // for a chord
    song_pointer->chord_pointers[chord_number]->insert_empty_chilren(
        first_child_number, number_of_children);
  }
  endInsertRows();
}

void ChordsModel::insert_json_children_directly(
    int first_child_number, const nlohmann::json &json_children,
    int chord_number) {
  beginInsertRows(
      get_chord_index(chord_number), first_child_number,
      first_child_number + static_cast<int>(json_children.size()) - 1);
  if (chord_number == -1) {
    // for root
    song_pointer->insert_json_chilren(first_child_number, json_children);
  } else {
    // for a chord
    song_pointer->chord_pointers[chord_number]->insert_json_chilren(
        first_child_number, json_children);
  }
  endInsertRows();
}

auto ChordsModel::insertRows(int first_child_number, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  undo_stack_pointer->push(std::make_unique<InsertEmptyChange>(
                               this, first_child_number, number_of_children,
                               get_chord_number(parent_index))
                               .release());
  return true;
}

auto ChordsModel::removeRows(int first_child_number, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  auto chord_number = get_chord_number(parent_index);
  undo_stack_pointer->push(
      std::make_unique<InsertRemoveChange>(
          this, first_child_number,
          copy_json_children(first_child_number, number_of_children,
                             chord_number),
          chord_number, false)
          .release());
  return true;
}

auto ChordsModel::copy_json_children(int first_child_number,
                                     int number_of_children,
                                     int chord_number) const -> nlohmann::json {
  return chord_number == -1
             // for root
             ? song_pointer->children_to_json(first_child_number,
                                              number_of_children)
             // for a chord
             : song_pointer->chord_pointers[chord_number]->children_to_json(
                   first_child_number, number_of_children);
}

void ChordsModel::insertJsonChildren(int first_child_number,
                                     const nlohmann::json &json_children,
                                     const QModelIndex &parent_index) {
  undo_stack_pointer->push(std::make_unique<InsertRemoveChange>(
                               this, first_child_number, json_children,
                               get_chord_number(parent_index), true)
                               .release());
}

auto ChordsModel::get_level(QModelIndex index) -> TreeLevel {
  // root will be an invalid index
  return index.row() == -1 ? root_level
         // chords have null parent pointers
         : index.internalPointer() == nullptr ? chord_level
                                              : note_level;
}

auto ChordsModel::verify_json_children(const QModelIndex &parent_index,
                                       const nlohmann::json &json_children)
    -> bool {
  auto parent_level = get_level(parent_index);
  if (parent_level == root_level) {
    static const nlohmann::json_schema::json_validator chords_validator(
        nlohmann::json({
            {"$schema", "http://json-schema.org/draft-07/schema#"},
            {"title", "Chords"},
            {"description", "a list of chords"},
            {"type", "array"},
            {"items", Chord::get_schema()},
        }));

    JsonErrorHandler error_handler;
    chords_validator.validate(json_children, error_handler);
    return !error_handler;
  }
  static const nlohmann::json_schema::json_validator notes_validator(
      nlohmann::json({{"$schema", "http://json-schema.org/draft-07/schema#"},
                      {"type", "array"},
                      {"title", "Notes"},
                      {"description", "the notes"},
                      {"items", Note::get_schema()}}));

  JsonErrorHandler error_handler;
  notes_validator.validate(json_children, error_handler);
  return !error_handler;
}

void ChordsModel::begin_reset_model() { beginResetModel(); }

void ChordsModel::end_reset_model() { endResetModel(); }

auto ChordsModel::copyJsonChildren(int first_child_number,
                                   int number_of_children,
                                   const QModelIndex &parent_index) const
    -> nlohmann::json {
  return copy_json_children(first_child_number, number_of_children,
                            get_chord_number(parent_index));
}

auto ChordsModel::get_text_color(bool is_default) -> QColor {
  return is_default ? DEFAULT_COLOR : NON_DEFAULT_COLOR;
}
