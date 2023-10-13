#include "models/ChordsModel.h"

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemModel
#include <qglobal.h>             // for QFlags
#include <qnamespace.h>          // for DisplayRole, ItemFlags, Orientation
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qundostack.h>          // for QUndoStack
#include <qvariant.h>            // for QVariant

#include <map>                           // for operator!=
#include <memory>                        // for make_unique, __unique_ptr_t
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>      // for basic_json
#include <nlohmann/json_fwd.hpp>  // for json
#include <vector>                 // for vector

#include "commands/CellChange.h"
#include "commands/InsertEmptyChange.h"
#include "commands/InsertRemoveChange.h"
#include "notechord/Note.h"
#include "notechord/NoteChord.h"  // for symbol_column, beats_column, instrument_...
#include "utilities/JsonErrorHandler.h"
#include "utilities/StableIndex.h"  // for StableIndex

class QObject;  // lines 19-19

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
  auto stable_index = get_stable_index(index);
  auto &chord_pointer = song_pointer->chord_pointers[stable_index.chord_number];
  auto note_number = stable_index.note_number;
  auto column_index = stable_index.column_index;
  auto cast_role = static_cast<Qt::ItemDataRole>(role);
  if (note_number == -1) {
    return chord_pointer->data(column_index, cast_role);
  }
  return chord_pointer->note_pointers[note_number]->data(column_index,
                                                         cast_role);
}

auto ChordsModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  auto column = index.column();
  if (column == symbol_column) {
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

// get a child index
auto ChordsModel::index(int row, int column,
                        const QModelIndex &parent_index) const -> QModelIndex {
  // createIndex needs a pointer to the item, not the parent
  // will error if row doesn't exist
  auto stable_parent_index = get_stable_index(parent_index);
  auto chord_number = stable_parent_index.chord_number;
  if (chord_number == -1) {
    return createIndex(row, column, nullptr);
  }
  auto &chord_pointer = song_pointer->chord_pointers[chord_number];
  auto note_number = stable_parent_index.note_number;
  if (note_number == -1) {
    return createIndex(row, column, chord_pointer.get());
  }
  return createIndex(row, column,
                     chord_pointer->note_pointers[chord_number].get());
}

// get the parent index
auto ChordsModel::parent(const QModelIndex &index) const -> QModelIndex {
  auto stable_index = get_stable_index(index);
  if (stable_index.chord_number == -1) {
    return {};
  }
  if (stable_index.note_number == -1) {
    return get_unstable_index(StableIndex({-1, -1, stable_index.column_index}));
  }
  return get_unstable_index(
      StableIndex({stable_index.chord_number, -1, stable_index.column_index}));
}

auto ChordsModel::rowCount(const QModelIndex &parent_index) const -> int {
  auto stable_parent_index = get_stable_index(parent_index);
  auto chord_number = stable_parent_index.chord_number;
  if (chord_number == -1) {
    return static_cast<int>(song_pointer->chord_pointers.size());
  }
  auto &chord_pointer = song_pointer->chord_pointers[chord_number];
  auto note_number = stable_parent_index.note_number;
  if (note_number == -1) {
    return static_cast<int>(chord_pointer->note_pointers.size());
  }
  return 0;
}

// node will check for errors, so no need to check for errors here
void ChordsModel::set_data_directly(const StableIndex &stable_index,
                                    const QVariant &new_value) {
  auto &chord_pointer = song_pointer->chord_pointers[stable_index.chord_number];
  auto note_number = stable_index.note_number;
  auto column_index = stable_index.column_index;
  if (note_number == -1) {
    chord_pointer->setData(column_index, new_value);
  }
  chord_pointer->note_pointers[note_number]->setData(column_index, new_value);
  auto index = get_unstable_index(stable_index);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, Qt::EditRole});
}

auto ChordsModel::setData(const QModelIndex &index, const QVariant &new_value,
                          int role) -> bool {
  if (role != Qt::EditRole) {
    return false;
  }
  undo_stack_pointer->push(
      std::make_unique<CellChange>(this, get_stable_index(index),
                                   data(index, Qt::EditRole), new_value)
          .release());
  return true;
}

void ChordsModel::remove_rows_directly(int first_child_number,
                                       int number_of_children,
                                       const StableIndex &stable_parent_index) {
  beginRemoveRows(get_unstable_index(stable_parent_index), first_child_number,
                  first_child_number + number_of_children - 1);
  auto chord_number = stable_parent_index.chord_number;
  if (chord_number == -1) {
    song_pointer->remove_chords(first_child_number, number_of_children);
  }
  song_pointer->chord_pointers[chord_number]->remove_notes(first_child_number,
                                                           number_of_children);
  endRemoveRows();
}

void ChordsModel::insert_empty_children_directly(
    int first_child_number, int number_of_children,
    const StableIndex &stable_parent_index) {
  beginInsertRows(get_unstable_index(stable_parent_index), first_child_number,
                  first_child_number + number_of_children - 1);
  auto chord_number = stable_parent_index.chord_number;
  if (chord_number == -1) {
    song_pointer->insert_empty_chords(first_child_number, number_of_children);
  } else {
    song_pointer->chord_pointers[chord_number]->insert_empty_notes(
        first_child_number, number_of_children);
  }
  endInsertRows();
}

auto ChordsModel::get_stable_index(const QModelIndex &index) const
    -> StableIndex {
  auto column_index = static_cast<NoteChordField>(index.column());
  auto *note_chord_pointer = static_cast<NoteChord *>(index.internalPointer());
  if (note_chord_pointer == nullptr) {
    return StableIndex({-1, -1, column_index});
  }
  auto level = note_chord_pointer->get_level();
  auto row = index.row();
  if (level == chord_level) {
    return StableIndex({row, -1, column_index});
  }
  auto *chord_pointer = note_chord_pointer->parent_pointer;
  auto chord_number = -1;
  auto &chord_pointers = song_pointer->chord_pointers;
  for (auto index = 0; index < static_cast<int>(chord_pointers.size());
       index = index + 1) {
    if (chord_pointers[index].get() == chord_pointer) {
      chord_number = index;
    }
  }
  return StableIndex({chord_number, row, column_index});
}

auto ChordsModel::get_unstable_index(const StableIndex &stable_index) const
    -> QModelIndex {
  auto chord_number = stable_index.chord_number;
  auto column_index = stable_index.column_index;
  if (chord_number == -1) {
    return {};
  }
  auto note_number = stable_index.note_number;
  if (note_number == -1) {
    return index(chord_number, column_index, QModelIndex());
  }
  return index(note_number, column_index,
               index(chord_number, 0, QModelIndex()));
};

void ChordsModel::insert_json_children_directly(
    int first_child_number, const nlohmann::json &insertion,
    const StableIndex &stable_parent_index) {
  beginInsertRows(get_unstable_index(stable_parent_index), first_child_number,
                  first_child_number + static_cast<int>(insertion.size()) - 1);
  auto chord_number = stable_parent_index.chord_number;
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
  undo_stack_pointer->push(std::make_unique<InsertEmptyChange>(
                               this, first_child_number, number_of_children,
                               get_stable_index(parent_index))
                               .release());
  return true;
}

auto ChordsModel::removeRows(int first_child_number, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  undo_stack_pointer->push(
      std::make_unique<InsertRemoveChange>(
          this, first_child_number,
          copy_json_children(first_child_number, number_of_children,
                             parent_index),
          get_stable_index(parent_index), false)
          .release());
  return true;
}

auto ChordsModel::copy_json_children(int first_child_number,
                                     int number_of_children,
                                     const QModelIndex &parent_index) const
    -> nlohmann::json {
  auto chord_number = get_stable_index(parent_index).chord_number;
  if (chord_number == -1) {
    return song_pointer->copy_json_chords(first_child_number,
                                          number_of_children);
  }
  return song_pointer->chord_pointers[chord_number]->copy_json_notes(
      first_child_number, number_of_children);
}

void ChordsModel::insertJsonChildren(int first_child_number,
                                     const nlohmann::json &insertion,
                                     const QModelIndex &parent_index) {
  undo_stack_pointer->push(
      std::make_unique<InsertRemoveChange>(this, first_child_number, insertion,
                                           get_stable_index(parent_index), true)
          .release());
}

auto ChordsModel::get_level(QModelIndex index) const -> TreeLevel {
  auto stable_index = get_stable_index(index);
  if (stable_index.chord_number == -1) {
    return root_level;
  }
  if (stable_index.note_number == -1) {
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