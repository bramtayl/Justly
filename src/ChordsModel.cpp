#include "ChordsModel.h"

#include <QtCore/qglobal.h>  // for QFlags, qCritical
#include <qjsonarray.h>      // for QJsonArray, QJsonArray::const_iterator
#include <qjsonobject.h>     // for QJsonObject
#include <qjsonvalue.h>      // for QJsonValueRef, QJsonValue, QJsonValueCon...
#include <qundostack.h>      // for QUndoStack

#include <algorithm>  // for copy, max, all_of
#include <iterator>   // for move_iterator, make_move_iterator
#include <utility>    // for move

#include "Chord.h"        // for Chord
#include "NoteChord.h"    // for NoteChord, symbol_column, beats_column
#include "StableIndex.h"  // for StableIndex
#include "Utilities.h"    // for error_column, error_instrument, error_level
#include "commands.h"     // for CellChange

class Instrument;
class QObject;  // lines 19-19

ChordsModel::ChordsModel(const std::vector<Instrument> &instruments_input,
                         QUndoStack &undo_stack_input, QObject *parent_input)
    : instruments(instruments_input),
      root(instruments_input),
      undo_stack(undo_stack_input),
      QAbstractItemModel(parent_input) {}

auto ChordsModel::columnCount(const QModelIndex & /*parent*/) const -> int {
  return NOTE_CHORD_COLUMNS;
}

auto ChordsModel::data(const QModelIndex &index, int role) const -> QVariant {
  // assume the index is valid because qt is requesting data for it
  const auto &node = const_node_from_index(index);
  if (!(node.verify_not_root())) {
    return {};
  }
  return node.note_chord_pointer->data(index.column(), role);
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
  auto &node = node_from_index(index);
  if (!(node.verify_not_root())) {
    return;
  }
  if (node.note_chord_pointer->setData(index.column(), new_value)) {
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
  }
}

auto ChordsModel::setData(const QModelIndex &index, const QVariant &new_value,
                          int role) -> bool {
  if (role != Qt::EditRole) {
    return false;
  }
  undo_stack.push(new CellChange(*this, index, new_value));
  return true;
}

auto ChordsModel::removeRows_no_signal(size_t position, size_t rows,
                                       const QModelIndex &parent_index)
    -> void {
  auto &parent_node = node_from_index(parent_index);
  if (!(parent_node.verify_child_at(position) &&
        parent_node.verify_child_at(position + rows - 1))) {
    return;
  };
  parent_node.child_pointers.erase(
      parent_node.child_pointers.begin() + static_cast<int>(position),
      parent_node.child_pointers.begin() + static_cast<int>(position + rows));
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
    size_t position, size_t rows, const QModelIndex &parent_index,
    std::vector<std::unique_ptr<TreeNode>> &deleted_rows) -> void {
  auto &node = node_from_index(parent_index);
  auto &child_pointers = node.child_pointers;
  if (!(node.verify_child_at(position) &&
        node.verify_child_at(position + rows - 1))) {
    return;
  };
  beginRemoveRows(parent_index, static_cast<int>(position),
                  static_cast<int>(position + rows - 1));
  deleted_rows.insert(
      deleted_rows.begin(),
      std::make_move_iterator(child_pointers.begin() +
                              static_cast<int>(position)),
      std::make_move_iterator(child_pointers.begin() +
                              static_cast<int>(position + rows)));
  auto &parent_node = node_from_index(parent_index);
  removeRows_no_signal(position, rows, parent_index);
  endRemoveRows();
}

auto ChordsModel::insertRows(int position, int rows,
                             const QModelIndex &parent_index) -> bool {
  auto &node = node_from_index(parent_index);
  auto &child_pointers = node.child_pointers;
  // will error if invalid
  if (!(node.verify_insertable_at(position))) {
    return false;
  };
  beginInsertRows(parent_index, position, position + rows - 1);
  for (int index = position; index < position + rows; index = index + 1) {
    // will error if childless
    child_pointers.insert(child_pointers.begin() + index,
                          std::make_unique<TreeNode>(instruments, &node));
  }
  endInsertRows();
  return true;
};

auto ChordsModel::insert_children(
    size_t position, std::vector<std::unique_ptr<TreeNode>> &insertion,
    const QModelIndex &parent_index) -> void {
  auto &parent_node = node_from_index(parent_index);

  if (!(parent_node.verify_insertable_at(position))) {
    return;
  }

  // will error if invalid
  auto child_level = parent_node.get_level() + 1;
  // make sure we are inserting the right level items
  for (const auto &new_child_pointer : insertion) {
    auto new_child_level = new_child_pointer->get_level();
    if (child_level != new_child_level) {
      // TODO: test
      qCritical("Level mismatch between level %d and new level %d!",
                child_level, new_child_level);
      return;
    }
  }

  beginInsertRows(parent_index, static_cast<int>(position),
                  static_cast<int>(position + insertion.size() - 1));
  auto &child_pointers = parent_node.child_pointers;
  child_pointers.insert(child_pointers.begin() + static_cast<int>(position),
                        std::make_move_iterator(insertion.begin()),
                        std::make_move_iterator(insertion.end()));
  insertion.clear();
  endInsertRows();
};

auto ChordsModel::save() const -> QJsonArray {
  QJsonArray json_chords;
  for (const auto &chord_node_pointer : root.child_pointers) {
    QJsonObject json_chord;
    chord_node_pointer->note_chord_pointer->save(json_chord);

    if (!(chord_node_pointer->child_pointers.empty())) {
      QJsonArray note_array;
      for (const auto &note_node_pointer : chord_node_pointer->child_pointers) {
        QJsonObject json_note;
        note_node_pointer->note_chord_pointer->save(json_note);
        note_array.push_back(std::move(json_note));
      }
      json_chord["notes"] = std::move(note_array);
    }
    json_chords.push_back(std::move(json_chord));
  }
  return json_chords;
}

void ChordsModel::load(const QJsonArray &json_chords) {
  beginResetModel();
  root.child_pointers.clear();
  for (const auto &chord_value : json_chords) {
    const auto &json_chord = chord_value.toObject();
    auto chord_node_pointer = std::make_unique<TreeNode>(instruments, &root);
    chord_node_pointer->note_chord_pointer->load(json_chord);

    if (json_chord.contains("notes")) {
      for (const auto &note_node : json_chord["notes"].toArray()) {
        const auto &json_note = note_node.toObject();
        auto note_node_pointer =
            std::make_unique<TreeNode>(instruments, chord_node_pointer.get());
        note_node_pointer->note_chord_pointer->load(json_note);
        chord_node_pointer->child_pointers.push_back(
            std::move(note_node_pointer));
      }
    }
    root.child_pointers.push_back(std::move(chord_node_pointer));
  }
  endResetModel();
}

auto ChordsModel::verify_json(const QJsonArray &json_chords,
                              const std::vector<Instrument> &instruments)
    -> bool {
  return std::all_of(json_chords.cbegin(), json_chords.cend(),
                     [&instruments](const auto &chord_value) {
                       if (!(verify_json_object(chord_value, "chord"))) {
                         return false;
                       }
                       const auto json_chord = chord_value.toObject();
                       return Chord::verify_json(json_chord, instruments);
                     });
}

auto ChordsModel::get_stable_index(const QModelIndex &index) const
    -> StableIndex {
  const auto &node = const_node_from_index(index);
  auto column = index.column();
  auto level = node.get_level();
  if (level == root_level) {
    return {-1, -1, column};
  }
  if (level == chord_level) {
    return {node.is_at_row(), -1, column};
  }
  if (level == note_level) {
    return {node.parent_pointer->is_at_row(), node.is_at_row(), column};
  }
  error_level(level);
  return {-1, -1, column};
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