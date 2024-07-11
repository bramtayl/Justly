#pragma once

#include <QAbstractItemModel>
#include <QItemSelectionModel>  // for QItemSelectionModel, QItemSelection...
#include <QObject>              // for Q_OBJECT

class TreeSelector : public QItemSelectionModel {
  Q_OBJECT
 public:
  explicit TreeSelector(QAbstractItemModel *model = nullptr);
  void select(const QItemSelection &new_selection,
              QItemSelectionModel::SelectionFlags command) override;
  void select(const QModelIndex &new_index,
              QItemSelectionModel::SelectionFlags command) override;
};