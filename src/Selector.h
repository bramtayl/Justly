#pragma once

#include <qitemselectionmodel.h>  // for QItemSelectionModel, QItemSelection...
class QAbstractItemModel;
class QObject;

class Selector : public QItemSelectionModel {
  public:
    Selector(QAbstractItemModel* model_pointer, QObject* parent_pointer);

  void select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command) override;
    
};