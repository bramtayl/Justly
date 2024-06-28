#include "models/InstrumentsModel.hpp"

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractListModel
#include <qassert.h>             // for Q_ASSERT
#include <qnamespace.h>          // for operator|, DisplayRole, EditRole
#include <qstring.h>             // for QString
#include <qvariant.h>            // for QVariant

#include <cstddef>  // for size_t
#include <vector>   // for vector

#include "justly/Instrument.hpp"  // for Instrument

class QObject;

InstrumentsModel::InstrumentsModel(bool include_empty_input,
                                   QObject *parent_pointer_input)
    : QAbstractListModel(parent_pointer_input),
      include_empty(include_empty_input) {}

auto InstrumentsModel::rowCount(const QModelIndex & /*parent*/) const -> int {
  Q_ASSERT(all_instruments_pointer != nullptr);
  return static_cast<int>(all_instruments_pointer->size());
}

auto InstrumentsModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  if (index.row() == 0 && !include_empty) {
    // disable empty option
    return {};
  }
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

auto InstrumentsModel::data(const QModelIndex &index, int role) const
    -> QVariant {
  auto row = index.row();
  Q_ASSERT(all_instruments_pointer != nullptr);
  Q_ASSERT(0 <= row);
  Q_ASSERT(static_cast<size_t>(row) < all_instruments_pointer->size());
  const auto &instrument = all_instruments_pointer->at(row);
  if (role == Qt::DisplayRole) {
    return QString::fromStdString(instrument.instrument_name);
  }
  if (role == Qt::EditRole) {
    return QVariant::fromValue(&instrument);
  }
  return {};
}
