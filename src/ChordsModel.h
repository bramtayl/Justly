#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemModel
#include <qjsonarray.h>
#include <qnamespace.h>          // for DisplayRole, ItemFlags, Orientation
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qvariant.h>            // for QVariant

#include <memory>  // for unique_ptr
#include <vector>  // for vector

#include "NoteChord.h"
#include "StableIndex.h"  // for StableIndex

class Instrument;
class QObject;  // lines 22-22
class QUndoStack;
class TreeNode;

class ChordsModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  TreeNode &root;
  const std::vector<Instrument> &instruments;
  QUndoStack &undo_stack;

  explicit ChordsModel(TreeNode &root,
                       const std::vector<Instrument> &instruments_input,
                       QUndoStack &undo_stack_input,
                       QObject *parent_input = nullptr);
  void begin_reset_model();
  void end_reset_model();

  [[nodiscard]] auto node_from_index(const QModelIndex &index) -> TreeNode &;
  [[nodiscard]] auto const_node_from_index(const QModelIndex &index) const
      -> const TreeNode &;
  [[nodiscard]] auto data(const QModelIndex &index, int role) const
      -> QVariant override;
  [[nodiscard]] auto flags(const QModelIndex &index) const
      -> Qt::ItemFlags override;
  [[nodiscard]] static auto column_flags(int column) -> Qt::ItemFlags;
  [[nodiscard]] auto headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const
      -> QVariant override;
  [[nodiscard]] auto index(int row, int column,
                           const QModelIndex &parent = QModelIndex()) const
      -> QModelIndex override;
  [[nodiscard]] auto parent(const QModelIndex &index) const
      -> QModelIndex override;
  [[nodiscard]] auto rowCount(const QModelIndex &parent = QModelIndex()) const
      -> int override;
  [[nodiscard]] auto columnCount(
      const QModelIndex &parent = QModelIndex()) const -> int override;
  auto copy(int position, int rows, const QModelIndex &parent_index,
            std::vector<std::unique_ptr<TreeNode>> &copy_to) -> int;
  void setData_irreversible(const QModelIndex &index,
                            const QVariant &new_value);
  auto insertRows(int position, int rows,
                  const QModelIndex &index = QModelIndex()) -> bool override;
  void insert_children(int position,
                       std::vector<std::unique_ptr<TreeNode>> &insertion,
                       const QModelIndex &parent_index);
  void removeRows_no_signal(int position, int rows,
                            const QModelIndex &index = QModelIndex());
  auto removeRows(int position, int rows,
                  const QModelIndex &index = QModelIndex()) -> bool override;
  void remove_save(int position, int rows, const QModelIndex &parent_index,
                   std::vector<std::unique_ptr<TreeNode>> &deleted_rows);

  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value, int role)
      -> bool override;

  [[nodiscard]] auto get_stable_index(const QModelIndex &index) const
      -> StableIndex;
  [[nodiscard]] auto get_unstable_index(const StableIndex &index) const
      -> QModelIndex;
  [[nodiscard]] auto get_level(const QModelIndex &index) -> TreeLevel;
  void insert_json_children(int position, const QJsonArray& inserted, const QModelIndex &parent_index);
  auto copy_json(int position, int rows, const QModelIndex &parent_index) -> QJsonArray;
  [[nodiscard]] auto verify_json_children(const QJsonArray& inserted, const QModelIndex &parent_index) const -> bool;
};
