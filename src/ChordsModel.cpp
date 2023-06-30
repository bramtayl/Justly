#include "ChordsModel.h"

#include <QtCore/qglobal.h>  // for QFlags, qCritical
#include <qundostack.h>      // for QUndoStack

#include <algorithm>  // for copy, max, all_of
#include <iterator>   // for move_iterator, make_move_iterator

#include "NoteChord.h"    // for NoteChord, symbol_column, beats_column
#include "StableIndex.h"  // for StableIndex
#include "TreeNode.h"        // for TreeNode
#include "commands.h"     // for CellChange
#include "utilities.h"    // for error_column, error_instrument, error_level

class Instrument;
class QObject;  // lines 19-19

ChordsModel::ChordsModel(TreeNode& root_input, const std::vector<Instrument> &instruments_input,
                         QUndoStack &undo_stack_input, QObject *parent_input)
    : instruments(instruments_input),
      root(root_input),
      undo_stack(undo_stack_input),
      QAbstractItemModel(parent_input) {}

auto ChordsModel::columnCount(const QModelIndex & /*parent*/) const -> int {
  return NOTE_CHORD_COLUMNS;
}

auto ChordsModel::data(const QModelIndex &index, int role) const -> QVariant {
  // assume the index is valid because qt is requesting data for it
  return const_node_from_index(index).data(index.column(), role);
}

auto ChordsModel::column_flags(int column) -> Qt::ItemFlags {
  if (column == symbol_column) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }
  if (column == interval_column || column == beats_column ||
      column == volume_percent_column || column == tempo_percent_column ||
      column == words_column || column == instrument_column) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
  }
  error_column(column);
  return Qt::NoItemFlags;
}

auto ChordsModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  const auto &node = const_node_from_index(index);
  if (!(node.verify_not_root())) {
    return {};
  }
  return column_flags(index.column());
}

auto ChordsModel::headerData(int section, Qt::Orientation orientation,
                             int role) const -> QVariant {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    if (section == symbol_column) {
      return {};
    }
    if (section == interval_column) {
      return "Interval";
    };
    if (section == beats_column) {
      return "Beats";
    };
    if (section == volume_percent_column) {
      return "Volume";
    };
    if (section == tempo_percent_column) {
      return "Tempo";
    };
    if (section == words_column) {
      return "Words";
    };
    if (section == instrument_column) {
      return "Instrument";
    };
    error_column(section);
  }
  // no horizontal headers
  // no headers for other roles
  return {};
}

auto ChordsModel::node_from_index(const QModelIndex &index) -> TreeNode & {
  if (!index.isValid()) {
    // an invalid index points to the root
    return root;
  }
  return *(static_cast<TreeNode *>(index.internalPointer()));
}

auto ChordsModel::const_node_from_index(const QModelIndex &index) const
    -> const TreeNode & {
  if (!index.isValid()) {
    // an invalid index points to the root
    return root;
  }
  return *(static_cast<TreeNode *>(index.internalPointer()));
}

// get a child index
auto ChordsModel::index(int row, int column,
                        const QModelIndex &parent_index) const -> QModelIndex {
  // createIndex needs a pointer to the item, not the parent
  // will error if row doesn't exist
  const auto &parent_node = const_node_from_index(parent_index);
  if (!(parent_node.verify_child_at(row))) {
    return {};
  }
  return createIndex(row, column, parent_node.child_pointers[row].get());
}

// get the parent index
auto ChordsModel::parent(const QModelIndex &index) const -> QModelIndex {
  const auto &node = const_node_from_index(index);
  if (!(node.verify_not_root())) {
    return {};
  }
  auto *parent_node_pointer = node.parent_pointer;
  if (parent_node_pointer->is_root()) {
    // root has an invalid index
    return {};
  }
  // always point to the nested first column of the parent
  return createIndex(parent_node_pointer->is_at_row(), 0, parent_node_pointer);
}

auto ChordsModel::rowCount(const QModelIndex &parent_index) const -> int {
  const auto &parent_node = const_node_from_index(parent_index);
  // column will be invalid for the root
  // we are only nesting into the first column of notes
  if (parent_node.is_root() || parent_index.column() == symbol_column) {
    return static_cast<int>(parent_node.get_child_count());
  }
  return 0;
}

// node will check for errors, so no need to check for errors here
void ChordsModel::setData_irreversible(const QModelIndex &index,
                                       const QVariant &new_value) {
  node_from_index(index).setData(index.column(), new_value);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

auto ChordsModel::setData(const QModelIndex &index, const QVariant &new_value,
                          int role) -> bool {
  if (role != Qt::EditRole) {
    return false;
  }
  undo_stack.push(
      std::make_unique<CellChange>(*this, index, new_value).release());
  return true;
}

auto ChordsModel::removeRows_no_signal(int position, int rows,
                                       const QModelIndex &parent_index)
    -> void {
  node_from_index(parent_index).remove_children(position, rows);
};

// node will check for errors, so no need to check here
auto ChordsModel::removeRows(int position, int rows,
                             const QModelIndex &parent_index) -> bool {
  beginRemoveRows(parent_index, position, position + rows - 1);
  removeRows_no_signal(position, rows, parent_index);
  endRemoveRows();
  return true;
};

// use additional deleted_rows to save deleted rows
// node will check for errors, so no need to check here
auto ChordsModel::remove_save(
    int position, int rows, const QModelIndex &parent_index,
    std::vector<std::unique_ptr<TreeNode>> &deleted_rows) -> void {
  beginRemoveRows(parent_index, position,
                  position + rows - 1);
  node_from_index(parent_index).remove_save_children(position, rows, deleted_rows);
  endRemoveRows();
}

auto ChordsModel::insertRows(int position, int rows,
                             const QModelIndex &parent_index) -> bool {
  beginInsertRows(parent_index, position, position + rows - 1);
  node_from_index(parent_index).insert_empty_children(position, rows);
  endInsertRows();
  return true;
};

auto ChordsModel::insert_children(
    int position, std::vector<std::unique_ptr<TreeNode>> &insertion,
    const QModelIndex &parent_index) -> void {
  beginInsertRows(parent_index, position,
                  static_cast<size_t>(position) + insertion.size() - 1);
  node_from_index(parent_index).insert_children(position, insertion);
  endInsertRows();
};

void ChordsModel::begin_reset_model() {
  beginResetModel();
}

void ChordsModel::end_reset_model() {
  endResetModel();
}

auto ChordsModel::get_stable_index(const QModelIndex &index) const
    -> StableIndex {
  return const_node_from_index(index).get_stable_index(index.column());
}

auto ChordsModel::get_unstable_index(const StableIndex &stable_index) const
    -> QModelIndex {
  auto chord_index = stable_index.chord_index;
  if (chord_index == -1) {
    return {};
  }
  auto note_index = stable_index.note_index;
  auto column_index = stable_index.column_index;
  if (note_index == -1) {
    return index(chord_index, column_index);
  }
  return index(note_index, column_index, index(chord_index, 0));
};