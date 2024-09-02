#pragma once

#include <cstddef>

#include <QItemSelectionModel>

class QAbstractItemModel;
class QModelIndex;

[[nodiscard]] auto get_first_child_number(const QItemSelectionRange &range) -> size_t;
[[nodiscard]] auto get_number_of_children(const QItemSelectionRange &range) -> size_t;
[[nodiscard]] auto is_rows(const QItemSelection &selection) -> bool;

struct TreeSelector : public QItemSelectionModel {
  explicit TreeSelector(QAbstractItemModel *model = nullptr);
  void select(const QItemSelection &new_selection,
              QItemSelectionModel::SelectionFlags command) override;
  void select(const QModelIndex &new_index,
              QItemSelectionModel::SelectionFlags command) override;
};