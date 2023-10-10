#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemModel
#include <qnamespace.h>          // for DisplayRole, ItemFlags, Orientation
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qvariant.h>            // for QVariant

#include <gsl/pointers>
#include <nlohmann/json_fwd.hpp>  // for json

#include "utilities/StableIndex.h"  // for StableIndex

class QObject;
class QUndoStack;
class TreeNode;

class ChordsModel : public QAbstractItemModel {
  Q_OBJECT
 private:
  gsl::not_null<TreeNode *> root_pointer;
  gsl::not_null<QUndoStack *> undo_stack_pointer;
  [[nodiscard]] auto get_stable_node(const StableIndex &stable_index) const
      -> TreeNode &;
  [[nodiscard]] auto get_unstable_index(const StableIndex &index) const
      -> QModelIndex;
  [[nodiscard]] auto get_stable_index(const QModelIndex &index) const
      -> StableIndex;

 public:
  explicit ChordsModel(gsl::not_null<TreeNode *> root_pointer_input,
                       gsl::not_null<QUndoStack *> undo_stack_pointer_input,
                       QObject *parent_pointer_input = nullptr);

  [[nodiscard]] auto rowCount(const QModelIndex &parent) const -> int override;
  [[nodiscard]] auto columnCount(const QModelIndex &parent) const
      -> int override;
  [[nodiscard]] auto parent(const QModelIndex &index) const
      -> QModelIndex override;
  [[nodiscard]] auto index(int row, int column, const QModelIndex &parent) const
      -> QModelIndex override;
  [[nodiscard]] auto headerData(int section, Qt::Orientation orientation,
                                int role) const -> QVariant override;
  [[nodiscard]] auto data(const QModelIndex &index, int role) const
      -> QVariant override;
  auto insertRows(int first_child_number, int number_of_children,
                  const QModelIndex &parent_index) -> bool override;
  auto removeRows(int first_child_number, int number_of_children,
                  const QModelIndex &parent_index) -> bool override;
  void insertJsonChildren(int first_child_number,
                          const nlohmann::json &insertion,
                          const QModelIndex &parent_index);
  [[nodiscard]] auto copy_json_children(int first_child_number,
                                      int number_of_children,
                                      const QModelIndex &parent_index) const
      -> nlohmann::json;
  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value, int role)
      -> bool override;
  [[nodiscard]] auto flags(const QModelIndex &index) const
      -> Qt::ItemFlags override;

  [[nodiscard]] auto get_node(const QModelIndex &index) const
      -> const TreeNode &;

  void load_from(const nlohmann::json &parsed_json);
  void set_data_directly(const StableIndex &index, const QVariant &new_value);
  void insert_json_children_directly(int first_child_number,
                                     const nlohmann::json &insertion,
                                     const StableIndex &parent_index);
  void remove_rows_directly(int first_child_number, int number_of_children,
                            const StableIndex &stable_parent_index);
  void insert_empty_children_directly(int first_child_number,
                                      int number_of_children,
                                      const StableIndex &stable_parent_index);
};
