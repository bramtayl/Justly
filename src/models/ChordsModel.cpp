#include "models/ChordsModel.h"

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemModel
#include <qglobal.h>             // for QFlags
#include <qnamespace.h>          // for DisplayRole, ItemFlags, Orientation
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qundostack.h>          // for QUndoStack
#include <qvariant.h>            // for QVariant

#include <map>                    // for operator!=
#include <memory>                 // for make_unique, __unique_ptr_t
#include <nlohmann/json.hpp>      // for basic_json
#include <nlohmann/json_fwd.hpp>  // for json
#include <vector>                 // for vector

#include "commands/CellChange.h"
#include "commands/InsertEmptyChange.h"
#include "commands/InsertRemoveChange.h"
#include "main/TreeNode.h"        // for TreeNode
#include "notechord/NoteChord.h"  // for symbol_column, beats_column, instrument_...
#include "utilities/StableIndex.h"  // for StableIndex

class QObject;  // lines 19-19

ChordsModel::ChordsModel(gsl::not_null<TreeNode *> root_pointer_input,
                         gsl::not_null<QUndoStack *> undo_stack_pointer_input,
                         QObject *parent_pointer_input)
    : QAbstractItemModel(parent_pointer_input),
      root_pointer(root_pointer_input),
      undo_stack_pointer(undo_stack_pointer_input) {}

auto ChordsModel::columnCount(const QModelIndex & /*parent*/) const -> int {
  return NOTE_CHORD_COLUMNS;
}

auto ChordsModel::data(const QModelIndex &index, int role) const -> QVariant {
  // assume the index is valid because qt is requesting data for it
  return get_node(index).data(static_cast<NoteChordField>(index.column()),
                              static_cast<Qt::ItemDataRole>(role));
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

auto ChordsModel::get_node(const QModelIndex &index) const -> const TreeNode & {
  if (!index.isValid()) {
    // an invalid index points to the root
    return *root_pointer;
  }
  return *(static_cast<TreeNode *>(index.internalPointer()));
}

// get a child index
auto ChordsModel::index(int row, int column,
                        const QModelIndex &parent_index) const -> QModelIndex {
  // createIndex needs a pointer to the item, not the parent
  // will error if row doesn't exist
  const auto &parent_node = get_node(parent_index);
  return createIndex(row, column, parent_node.get_child_pointers()[row].get());
}

// get the parent index
auto ChordsModel::parent(const QModelIndex &index) const -> QModelIndex {
  const auto &parent_node = get_node(index).get_const_parent();
  if (parent_node.is_root()) {
    // parent is root so has invalid index
    return {};
  }
  // always point to the nested first column of the parent
  return createIndex(parent_node.get_row(), 0, &parent_node);
}

auto ChordsModel::rowCount(const QModelIndex &parent_index) const -> int {
  const auto &parent_node = get_node(parent_index);
  // column will be invalid for the root
  // we are only nesting into the symbol column
  if (parent_node.is_root() || parent_index.column() == symbol_column) {
    return static_cast<int>(parent_node.number_of_children());
  }
  return 0;
}

// node will check for errors, so no need to check for errors here
void ChordsModel::set_data_directly(const StableIndex &stable_index,
                                    const QVariant &new_value) {
  auto index = get_unstable_index(stable_index);
  get_stable_node(stable_index).setData(stable_index.column_index, new_value);
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
  get_stable_node(stable_parent_index)
      .remove_children(first_child_number, number_of_children);
  endRemoveRows();
}

void ChordsModel::insert_empty_children_directly(
    int first_child_number, int number_of_children,
    const StableIndex &stable_parent_index) {
  beginInsertRows(get_unstable_index(stable_parent_index), first_child_number,
                  first_child_number + number_of_children - 1);
  get_stable_node(stable_parent_index)
      .insert_empty_children(first_child_number, number_of_children);
  endInsertRows();
}

auto ChordsModel::get_stable_node(const StableIndex &stable_index) const
    -> TreeNode & {
  auto chord_number = stable_index.chord_number;
  if (chord_number == -1) {
    return *root_pointer;
  }
  auto note_number = stable_index.note_number;
  auto &chord = *(root_pointer->get_child_pointers()[chord_number]);
  if (note_number == -1) {
    return chord;
  }
  return *(chord.get_child_pointers()[note_number]);
}

auto ChordsModel::get_stable_index(const QModelIndex &index) const
    -> StableIndex {
  return get_node(index).get_stable_index(
      static_cast<NoteChordField>(index.column()));
}

auto ChordsModel::get_unstable_index(const StableIndex &stable_index) const
    -> QModelIndex {
  auto chord_number = stable_index.chord_number;
  if (chord_number == -1) {
    return {};
  }
  auto note_number = stable_index.note_number;
  auto column_index = stable_index.column_index;
  if (note_number == -1) {
    return index(chord_number, column_index, QModelIndex());
  }
  return index(note_number, column_index, index(chord_number, 0, QModelIndex()));
};

void ChordsModel::insert_json_children_directly(
    int first_child_number, const nlohmann::json &insertion,
    const StableIndex &stable_parent_index) {
  beginInsertRows(get_unstable_index(stable_parent_index), first_child_number,
                  first_child_number + static_cast<int>(insertion.size()) - 1);
  get_stable_node(stable_parent_index)
      .insert_json_children(first_child_number, insertion);
  endInsertRows();
}

void ChordsModel::load_from(const nlohmann::json &parsed_json) {
  beginResetModel();
  root_pointer->load_from(parsed_json);
  endResetModel();
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
  return get_node(parent_index)
      .copy_json_children(first_child_number, number_of_children);
}

void ChordsModel::insertJsonChildren(int first_child_number,
                                     const nlohmann::json &insertion,
                                     const QModelIndex &parent_index) {
  undo_stack_pointer->push(
      std::make_unique<InsertRemoveChange>(this, first_child_number, insertion,
                                           get_stable_index(parent_index), true)
          .release());
}