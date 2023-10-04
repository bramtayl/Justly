#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemModel
#include <qnamespace.h>          // for DisplayRole, ItemFlags, Orientation
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qvariant.h>            // for QVariant

#include <gsl/pointers>
#include <memory>                 // for unique_ptr
#include <nlohmann/json_fwd.hpp>  // for json
#include <vector>                 // for vector

#include "utilities/StableIndex.h"  // for StableIndex

class QObject;
class TreeNode;

class ChordsModel : public QAbstractItemModel {
  Q_OBJECT
 private:
  gsl::not_null<TreeNode *> root_pointer;

 public:
  explicit ChordsModel(gsl::not_null<TreeNode *> root_pointer,
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

  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value, int role)
      -> bool override;
  [[nodiscard]] auto flags(const QModelIndex &index) const
      -> Qt::ItemFlags override;

  auto insertRows(int first_index, int number_of_children,
                                const QModelIndex &index) -> bool override;
  auto removeRows(int first_index, int number_of_children,
                                const QModelIndex &index) -> bool override;

  [[nodiscard]] auto get_stable_index(const QModelIndex &index) const
      -> StableIndex;
  [[nodiscard]] auto get_unstable_index(const StableIndex &index) const
      -> QModelIndex;

  [[nodiscard]] auto get_node(const QModelIndex &index) const -> TreeNode &;
  [[nodiscard]] auto get_const_node(const QModelIndex &index) const
      -> const TreeNode &;

  void directly_set_data(const QModelIndex &index, const QVariant &new_value);

  void insert_children(
      int first_index,
      std::vector<std::unique_ptr<TreeNode>> *insertion_pointer,
      const QModelIndex &parent_index);
  void insert_json_children(int first_index, const nlohmann::json &insertion,
                            const QModelIndex &parent_index);
  void remove_save(
      int first_index, int number_of_children, const QModelIndex &parent_index,
      std::vector<std::unique_ptr<TreeNode>> *deleted_children_pointer);
  void load_from(const nlohmann::json& parsed_json);

 signals:
  void about_to_set_data(const QModelIndex &index, QVariant old_value,
                         QVariant new_value);
};
