#pragma once

#include <qtmetamacros.h>  // for Q_OBJECT

#include "qitemselectionmodel.h"  // for QItemSelectionModel, QItemSelection...

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