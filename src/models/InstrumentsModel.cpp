#include "models/InstrumentsModel.hpp"

#include <qabstractitemmodel.h>  // for QAbstractListModel, QModelIndex
#include <qnamespace.h>          // for DisplayRole, EditRole, ItemDataRole
#include <qstring.h>             // for QString
#include <qvariant.h>            // for QVariant

#include <vector>   // for vector

#include "justly/Instrument.hpp"  // for Instrument
#include "song/instruments.hpp"

class QObject;

InstrumentsModel::InstrumentsModel(bool include_empty_input,
                                   QObject *parent_pointer_input)
    : QAbstractListModel(parent_pointer_input),
      include_empty(include_empty_input) {}

auto InstrumentsModel::rowCount(const QModelIndex & /*parent*/) const -> int {
  return static_cast<int>(get_all_instruments().size());
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
  const auto& instruments = get_all_instruments();
  Q_ASSERT(0 <= row);
  Q_ASSERT(static_cast<size_t>(row) < instruments.size());
  const auto &instrument = instruments[row];
  if (role == Qt::DisplayRole) {
    return QString::fromStdString(instrument.instrument_name);
  }
  if (role == Qt::EditRole) {
    return QVariant::fromValue(&instrument);
  }
  return {};
}
