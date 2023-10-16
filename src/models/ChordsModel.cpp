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
  if (note_number == -1) {
    // the parent is the root, which doesn't exist as an object
    return createIndex(chord_number, note_chord_field, nullptr);
  }
  // its a note, so the parent is its chord
  return createIndex(note_number, note_chord_field,
                     song_pointer->chord_pointers[chord_number].get());
};

auto ChordsModel::get_song_index(const QModelIndex &index) const -> SongIndex {
  auto child_number = index.row();
  if (child_number == -1) {
    // its root
    return SongIndex({-1, -1, symbol_column});
  }
  auto note_chord_field = static_cast<NoteChordField>(index.column());
  // the parent pointer is a pointer to the chord or null
  auto *chord_pointer = static_cast<Chord *>(index.internalPointer());
  if (chord_pointer == nullptr) {
    // it's chord, which doesn't have a pointer
    return SongIndex({child_number, -1, note_chord_field});
  }
  auto &chord_pointers = song_pointer->chord_pointers;
  auto chord_number = -1;
  for (auto maybe_chord_number = 0;
       maybe_chord_number < static_cast<int>(chord_pointers.size());
       // look for the chord in
       maybe_chord_number = maybe_chord_number + 1) {
    if (chord_pointers[maybe_chord_number].get() == chord_pointer) {
      chord_number = maybe_chord_number;
    }
  }
  return SongIndex({chord_number, child_number, note_chord_field});
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
  // assume the index is valid because qt is requesting data for it
  auto song_index = get_song_index(index);
  auto chord_number = song_index.chord_number;
  auto &chord_pointer = song_pointer->chord_pointers[chord_number];
  auto note_number = song_index.note_number;
  auto note_chord_field = song_index.note_chord_field;
  auto cast_role = static_cast<Qt::ItemDataRole>(role);
  if (note_number == -1) {
    return chord_pointer->data(note_chord_field, cast_role);
  }
  return chord_pointer->note_pointers[note_number]->data(note_chord_field,
                                                         cast_role);
}

auto ChordsModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  auto note_chord_field = index.column();
  if (note_chord_field == symbol_column) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
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
  auto song_index = get_song_index(index);
  if (song_index.note_number == -1) {
    return get_tree_index(SongIndex({-1, -1, symbol_column}));
  }
  return get_tree_index(
      SongIndex({song_index.chord_number, -1, symbol_column}));
}

// get a child index
auto ChordsModel::index(int child_number, int note_chord_field,
                        const QModelIndex &parent_index) const -> QModelIndex {
  // createIndex needs a pointer to the item, not the parent
  // will error if row doesn't exist
  auto parent_song_index = get_song_index(parent_index);
  auto cast_note_chord_field = static_cast<NoteChordField>(note_chord_field);
  auto chord_number = parent_song_index.chord_number;
  if (chord_number == -1) {
    return get_tree_index(SongIndex({child_number, -1, cast_note_chord_field}));
  }
  return get_tree_index(
      SongIndex({chord_number, child_number, cast_note_chord_field}));
}

auto ChordsModel::rowCount(const QModelIndex &parent_index) const -> int {
  auto song_index = get_song_index(parent_index);
  auto chord_number = song_index.chord_number;
  if (song_index.note_chord_field == symbol_column) {
    if (chord_number == -1) {
      return static_cast<int>(song_pointer->chord_pointers.size());
    }
    auto &chord_pointer = song_pointer->chord_pointers[chord_number];
    auto note_number = song_index.note_number;
    if (note_number == -1) {
      return static_cast<int>(chord_pointer->note_pointers.size());
    }
  }
  return 0;
}

// node will check for errors, so no need to check for errors here
void ChordsModel::set_data_directly(const SongIndex &song_index,
                                    const QVariant &new_value) {
  auto &chord_pointer = song_pointer->chord_pointers[song_index.chord_number];
  auto note_number = song_index.note_number;
  auto note_chord_field = song_index.note_chord_field;
  if (note_number == -1) {
    chord_pointer->setData(note_chord_field, new_value);
  } else {
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

void ChordsModel::remove_rows_directly(int first_child_number,
                                       int number_of_children,
                                       const SongIndex &parent_song_index) {
  beginRemoveRows(get_tree_index(parent_song_index), first_child_number,
                  first_child_number + number_of_children - 1);
  auto chord_number = parent_song_index.chord_number;
  if (chord_number == -1) {
    song_pointer->remove_chords(first_child_number, number_of_children);
  } else {
    song_pointer->chord_pointers[chord_number]->remove_notes(
        first_child_number, number_of_children);
  }
  endRemoveRows();
}

void ChordsModel::insert_empty_children_directly(
    int first_child_number, int number_of_children,
    const SongIndex &parent_song_index) {
  beginInsertRows(get_tree_index(parent_song_index), first_child_number,
                  first_child_number + number_of_children - 1);
  auto chord_number = parent_song_index.chord_number;
  if (chord_number == -1) {
    song_pointer->insert_empty_chords(first_child_number, number_of_children);
  } else {
    song_pointer->chord_pointers[chord_number]->insert_empty_notes(
        first_child_number, number_of_children);
  }
  endInsertRows();
}

void ChordsModel::insert_json_children_directly(
    int first_child_number, const nlohmann::json &insertion,
    const SongIndex &parent_song_index) {
  beginInsertRows(get_tree_index(parent_song_index), first_child_number,
                  first_child_number + static_cast<int>(insertion.size()) - 1);
  auto chord_number = parent_song_index.chord_number;
  if (chord_number == -1) {
    song_pointer->insert_json_chords(first_child_number, insertion);
  } else {
    song_pointer->chord_pointers[chord_number]->insert_json_notes(
        first_child_number, insertion);
  }
  endInsertRows();
}

auto ChordsModel::insertRows(int first_child_number, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  undo_stack_pointer->push(gsl::not_null(
      new InsertEmptyChange(this, first_child_number, number_of_children,
                            get_song_index(parent_index))));
  return true;
}

auto ChordsModel::removeRows(int first_child_number, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  undo_stack_pointer->push(gsl::not_null(new InsertRemoveChange(
      this, first_child_number,
      copy_json_children(first_child_number, number_of_children, parent_index),
      get_song_index(parent_index), false)));
  return true;
}

auto ChordsModel::copy_json_children(int first_child_number,
                                     int number_of_children,
                                     const QModelIndex &parent_index) const
    -> nlohmann::json {
  auto chord_number = get_song_index(parent_index).chord_number;
  if (chord_number == -1) {
    return song_pointer->chords_to_json(first_child_number,
                                          number_of_children);
  }
  return song_pointer->chord_pointers[chord_number]->notes_to_json(
      first_child_number, number_of_children);
}

void ChordsModel::insertJsonChildren(int first_child_number,
                                     const nlohmann::json &insertion,
                                     const QModelIndex &parent_index) {
  undo_stack_pointer->push(gsl::not_null(
      new InsertRemoveChange(this, first_child_number, insertion,
                             get_song_index(parent_index), true)));
}

auto ChordsModel::get_level(QModelIndex index) const -> TreeLevel {
  auto song_index = get_song_index(index);
  if (song_index.chord_number == -1) {
    return root_level;
  }
  if (song_index.note_number == -1) {
    return chord_level;
  }
  return note_level;
}

auto ChordsModel::verify_json_children(const QModelIndex &parent_index,
                                       const nlohmann::json &paste_json) const
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
    chords_validator.validate(paste_json, error_handler);
    return !error_handler;
  }
  static const nlohmann::json_schema::json_validator notes_validator(
      nlohmann::json({{"$schema", "http://json-schema.org/draft-07/schema#"},
                      {"type", "array"},
                      {"title", "Notes"},
                      {"description", "the notes"},
                      {"items", Note::get_schema()}}));

  JsonErrorHandler error_handler;
  notes_validator.validate(paste_json, error_handler);
  return !error_handler;
}

void ChordsModel::begin_reset_model() { beginResetModel(); }

void ChordsModel::end_reset_model() { endResetModel(); }
