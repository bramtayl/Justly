#include "ChordsModel.h"

#include <QtCore/qglobal.h>  // for QFlags
#include <qjsonarray.h>
#include <qundostack.h>      // for QUndoStack

#include "Editor.h"
#include "NoteChord.h"    // for symbol_column, beats_column, instrument_...
#include "StableIndex.h"  // for StableIndex
#include "TreeNode.h"     // for TreeNode
#include "commands.h"     // for SetData
#include "utilities.h"    // for error_column

class QObject;  // lines 19-19
class Song;

ChordsModel::ChordsModel(TreeNode &root_input,
                         Editor &editor_input, QObject *parent_pointer_input)
    : root(root_input),
      editor(editor_input),
      QAbstractItemModel(parent_pointer_input) {}

auto ChordsModel::columnCount(const QModelIndex & /*parent*/) const -> int {
  return NOTE_CHORD_COLUMNS;
}

auto ChordsModel::data(const QModelIndex &index, int role) const -> QVariant {
  // assume the index is valid because qt is requesting data for it
  return const_node_from_index(index).data(index.column(), role);
}

// separate out to test more easily
auto ChordsModel::column_flags(int column) -> Qt::ItemFlags {
  if (column == symbol_column) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }
  if (column == interval_column || column == beats_column ||
      column == volume_percent_column || column == tempo_percent_column ||
      column == words_column || column == instrument_column) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
  }
  return Qt::NoItemFlags;
}

auto ChordsModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  return column_flags(index.column());
}

auto ChordsModel::headerData(int section, Qt::Orientation orientation,
                             int role) const -> QVariant {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    if (section == symbol_column) {
      return {};
    }
    if (section == interval_column) {
      return tr("Interval");
    };
    if (section == beats_column) {
      return tr("Beats");
    };
    if (section == volume_percent_column) {
      return tr("Volume");
    };
    if (section == tempo_percent_column) {
      return tr("Tempo");
    };
    if (section == words_column) {
      return tr("Words");
    };
    if (section == instrument_column) {
      return tr("Instrument");
    };
  }
  // no horizontal headers
  // no headers for other roles
  return {};
}

auto ChordsModel::get_node(const QModelIndex &index) -> TreeNode & {
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
    // parent is root so has invalid index
    return {};
  }
  // always point to the nested first column of the parent
  return createIndex(parent_node_pointer->get_row(), 0, parent_node_pointer);
}

auto ChordsModel::rowCount(const QModelIndex &parent_index) const -> int {
  const auto &parent_node = const_node_from_index(parent_index);
  // column will be invalid for the root
  // we are only nesting into the symbol column
  if (parent_node.is_root() || parent_index.column() == symbol_column) {
    return static_cast<int>(parent_node.number_of_children());
  }
  return 0;
}

// node will check for errors, so no need to check for errors here
void ChordsModel::directly_set_data(const QModelIndex &index,
                                       const QVariant &new_value) {
  get_node(index).setData(index.column(), new_value);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

auto ChordsModel::setData(const QModelIndex &index, const QVariant &new_value,
                          int role) -> bool {
  if (role != Qt::EditRole) {
    return false;
  }
  editor.undo_stack.push(
      std::make_unique<SetData>(editor, index, new_value).release());
  return true;
}

// node will check for errors, so no need to check here
auto ChordsModel::removeRows(int first_index, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  beginRemoveRows(parent_index, first_index, first_index + number_of_children - 1);
  get_node(parent_index).remove_children(first_index, number_of_children);
  endRemoveRows();
  return true;
};

// use additional deleted_children to save deleted rows
// node will check for errors, so no need to check here
auto ChordsModel::remove_save(
    int first_index, int number_of_children, const QModelIndex &parent_index,
    std::vector<std::unique_ptr<TreeNode>> &deleted_children) -> void {
  beginRemoveRows(parent_index, first_index, first_index + number_of_children - 1);
  get_node(parent_index)
      .remove_save_children(first_index, number_of_children, deleted_children);
  endRemoveRows();
}

auto ChordsModel::insertRows(int first_index, int number_of_children,
                             const QModelIndex &parent_index) -> bool {
  beginInsertRows(parent_index, first_index, first_index + number_of_children - 1);
  get_node(parent_index).insert_empty_children(first_index, number_of_children);
  endInsertRows();
  return true;
};

auto ChordsModel::insert_children(
    int first_index, std::vector<std::unique_ptr<TreeNode>> &insertion,
    const QModelIndex &parent_index) -> void {
  beginInsertRows(parent_index, first_index,
                  first_index + static_cast<int>(insertion.size()) - 1);
  get_node(parent_index).insert_children(first_index, insertion);
  endInsertRows();
};

void ChordsModel::begin_reset_model() { beginResetModel(); }

void ChordsModel::end_reset_model() { endResetModel(); }

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

auto ChordsModel::get_level(const QModelIndex &index) -> TreeLevel {
  return get_node(index).get_level();
}

auto ChordsModel::copy_json(int first_index, int number_of_children, const QModelIndex &parent_index) -> QJsonArray {
  return get_node(parent_index).copy_json_children(first_index, number_of_children);
}

void ChordsModel::insert_json_children(int first_index, const QJsonArray& insertion, const QModelIndex &parent_index) {
  beginInsertRows(parent_index, first_index,
                  first_index + static_cast<int>(insertion.size()) - 1);
  get_node(parent_index).insert_json_children(first_index, insertion);
  endInsertRows();
}

auto ChordsModel::verify_json_children(const Song& song, const QJsonArray& insertion, const QModelIndex &parent_index) const -> bool {
  return const_node_from_index(parent_index).verify_json_children(song, insertion);
}