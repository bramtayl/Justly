#pragma once

#include <QItemSelectionModel>
#include <QObject>

class QAbstractItemModel;
class QModelIndex;

class TreeSelector : public QItemSelectionModel {
  Q_OBJECT
public:
  explicit TreeSelector(QAbstractItemModel *model = nullptr);
  void select(const QItemSelection &new_selection,
              QItemSelectionModel::SelectionFlags command) override;
  void select(const QModelIndex &new_index,
              QItemSelectionModel::SelectionFlags command) override;
};