#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex (ptr only), QAbstractLis...
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qvariant.h>            // for QVariant

class QObject;

class InstrumentsModel : public QAbstractListModel {
  Q_OBJECT
 private:
  bool include_empty;

 public:
  explicit InstrumentsModel(bool include_empty,
                            QObject *parent_pointer_input = nullptr);
  [[nodiscard]] auto data(const QModelIndex &index, int role) const
      -> QVariant override;
  [[nodiscard]] auto rowCount(const QModelIndex &parent) const -> int override;
};
