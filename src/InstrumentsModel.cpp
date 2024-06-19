#include "src/InstrumentsModel.h"

#include <qabstractitemmodel.h>  // for QAbstractListModel, QModelIndex
#include <qnamespace.h>          // for DisplayRole, EditRole, ItemDataRole
#include <qstring.h>             // for QString
#include <qvariant.h>            // for QVariant

#include <cstddef>  // for size_t
#include <vector>   // for vector

#include "justly/Instrument.h"  // for Instrument
#include "src/instruments.h"

class QObject;

InstrumentsModel::InstrumentsModel(bool include_empty_input,
                                   QObject *parent_pointer_input)
    : QAbstractListModel(parent_pointer_input),
      include_empty(include_empty_input) {}

auto InstrumentsModel::data(const QModelIndex &index, int role) const
    -> QVariant {
  const auto &instrument =
      get_all_instruments().at(index.row());
  if (role == Qt::DisplayRole) {
    return QString::fromStdString(instrument.instrument_name);
  }
  if (role == Qt::EditRole) {
    return QVariant::fromValue(&instrument);
  }
  return {};
}

auto InstrumentsModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  if (index.row() == 0 && !include_empty) {
    return {};
  }
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

auto InstrumentsModel::rowCount(const QModelIndex & /*parent*/) const -> int {
  return static_cast<int>(get_all_instruments().size());
}
