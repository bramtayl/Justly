#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex (ptr only), QAbstractLis...
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qvariant.h>            // for QVariant

#include "justly/macros.h"

class QObject;

class InstrumentsModel : public QAbstractListModel {
  Q_OBJECT
  bool include_empty;

 public:
  explicit InstrumentsModel(bool, QObject* = nullptr);
  NO_MOVE_COPY(InstrumentsModel);
  [[nodiscard]] auto data(const QModelIndex& index, int role) const
      -> QVariant override;
  [[nodiscard]] auto rowCount(const QModelIndex& /*parent*/) const
      -> int override;
};
