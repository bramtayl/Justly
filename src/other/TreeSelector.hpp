#pragma once

#include <QItemSelectionModel>

class QAbstractItemModel;
class QModelIndex;

struct TreeSelector : public QItemSelectionModel {
  explicit TreeSelector(QAbstractItemModel *model = nullptr);
  void select(const QItemSelection &new_selection,
              QItemSelectionModel::SelectionFlags command) override;
  void select(const QModelIndex &new_index,
              QItemSelectionModel::SelectionFlags command) override;
};