#include "models/ChordsModel.h"

#include <QtCore/qglobal.h>      // for QFlags
#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemM...
#include <qnamespace.h>          // for EditRole, operator|, Displa...
#include <qtmetamacros.h>        // for emit
#include <qundostack.h>          // for QUndoStack
#include <qvariant.h>            // for QVariant

#include <map>                           // for operator!=
#include <memory>                        // for make_unique, unique_ptr
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json-schema.hpp>      // for json_validator
#include <nlohmann/json.hpp>             // for basic_json<>::object_t, bas...
#include <nlohmann/json_fwd.hpp>         // for json
#include <vector>                        // for vector

#include "commands/CellChange.h"          // for CellChange
#include "commands/InsertEmptyChange.h"   // for InsertEmptyChange
#include "commands/InsertRemoveChange.h"  // for InsertRemoveChange
#include "main/Song.h"                    // for Song
#include "notechord/Chord.h"              // for Chord
#include "notechord/Note.h"               // for Note
#include "notechord/NoteChord.h"          // for symbol_column, NoteChordField
#include "utilities/JsonErrorHandler.h"   // for JsonErrorHandler
#include "utilities/SongIndex.h"          // for SongIndex

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
  auto level = ChordsModel::get_level(index);
  if (level == root_level) {
    // its root, so no chord number
    return -1;
  }
  if (level == chord_level) {
    // the chord number is the index row
    return index.row();
  }
  // for notes, we need to search for the chord number
  auto *chord_pointer = index.internalPointer();
  auto &chord_pointers = song_pointer->chord_pointers;
  int chord_number = -1;
  for (auto maybe_chord_number = 0;
       maybe_chord_number < static_cast<int>(chord_pointers.size());
       // look for the chord in
       maybe_chord_number = maybe_chord_number + 1) {
    if (chord_pointers[maybe_chord_number].get() == chord_pointer) {
      chord_number = maybe_chord_number;
    }
  }
  return chord_number;
}

ChordsModel::ChordsModel(gsl::not_null<Song *> song_pointer_input,
                         gsl::not_null<QUndoStack *> undo_stack_pointer_input,
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
  auto cast_role = static_cast<Qt::ItemDataRole>(role);
  // for chords
  return note_number == -1 ? chord_pointer->data(note_chord_field, cast_role)
                           // for notes
                           : chord_pointer->note_pointers[note_number]->data(
                                 note_chord_field, cast_role);
}

auto ChordsModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  return Qt::ItemIsEnabled | (index.column() == symbol_column
                                  ? Qt::ItemIsSelectable
                                  : Qt::ItemIsSelectable | Qt::ItemIsEditable);
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
  if (note_number == -1) {
    // for a chord
    chord_pointer->setData(note_chord_field, new_value);
  } else {
    // for a note
    chord_pointer->note_pointers[note_number]->setData(note_chord_field,
                                                       new_value);
  }
  auto index = get_tree_index(song_index);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, Qt::EditRole});
}

auto ChordsModel::setData(const QModelIndex &index, const QVariant &new_value,
                          int role) -> bool {
  if (role != Qt::EditRole) {
    return false;
  }
  undo_stack_pointer->push(gsl::not_null(new CellChange(
      this, get_song_index(index), data(index, Qt::EditRole), new_value)));
  return true;
}

void ChordsModel::remove_children_directly(int first_child_number,
                                           int number_of_children,
                                           int chord_number) {
  beginRemoveRows(get_chord_index(chord_number), first_child_number,
                  first_child_number + number_of_children - 1);
  if (chord_number == -1) {
    // for root
    song_pointer->remove_chords(first_child_number, number_of_children);
  } else {
    // for a chord
    song_pointer->chord_pointers[chord_number]->remove_notes(
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
    song_pointer->insert_empty_chords(first_child_number, number_of_children);
  } else {
    // for a chord
    song_pointer->chord_pointers[chord_number]->insert_empty_notes(
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
    song_pointer->insert_json_chords(first_child_number, json_children);
  } else {
    // for a chord
    song_pointer->chord_pointers[chord_number]->insert_json_notes(
        first_child_number, json_children);
  }
  endInsertRows();
}

auto ChordsModel::insertRows(int first_child_number, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  undo_stack_pointer->push(gsl::not_null(
      new InsertEmptyChange(this, first_child_number, number_of_children,
                            get_chord_number(parent_index))));
  return true;
}

auto ChordsModel::removeRows(int first_child_number, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  auto chord_number = get_chord_number(parent_index);
  undo_stack_pointer->push(gsl::not_null(new InsertRemoveChange(
      this, first_child_number,
      copy_json_children(first_child_number, number_of_children, chord_number),
      chord_number, false)));
  return true;
}

auto ChordsModel::copy_json_children(int first_child_number,
                                     int number_of_children,
                                     int chord_number) const -> nlohmann::json {
  return chord_number == -1
             // for root
             ? song_pointer->chords_to_json(first_child_number,
                                            number_of_children)
             // for a chord
             : song_pointer->chord_pointers[chord_number]->notes_to_json(
                   first_child_number, number_of_children);
}

void ChordsModel::insertJsonChildren(int first_child_number,
                                     const nlohmann::json &json_children,
                                     const QModelIndex &parent_index) {
  undo_stack_pointer->push(gsl::not_null(
      new InsertRemoveChange(this, first_child_number, json_children,
                             get_chord_number(parent_index), true)));
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
