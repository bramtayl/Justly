#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex (ptr only), QAbstractLi...
#include <qnamespace.h>          // for ItemFlags
#include <qobject.h>
#include <qtmetamacros.h>  // for Q_OBJECT
#include <qvariant.h>      // for QVariant

#include <vector>  // for vector

#include "justly/Instrument.hpp"  // for get_all_instruments

class InstrumentsModel : public QAbstractListModel {
  Q_OBJECT
  const bool include_empty;
  const std::vector<Instrument>* const all_instruments_pointer =
      &get_all_instruments();

 public:
  explicit InstrumentsModel(bool include_empty_input,
                            QObject* parent_pointer_input = nullptr);

  [[nodiscard]] auto rowCount(const QModelIndex& /*parent*/) const
      -> int override;
  [[nodiscard]] auto flags(const QModelIndex& index) const
      -> Qt::ItemFlags override;
  [[nodiscard]] auto data(const QModelIndex& index, int role) const
      -> QVariant override;
};
