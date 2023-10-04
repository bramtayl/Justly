#include "models/InstrumentsModel.h"

#include <qabstractitemmodel.h>  // for QAbstractListModel, QModelIndex
#include <qnamespace.h>          // for DisplayRole
#include <qvariant.h>            // for QVariant

#include <vector>  // for vector

#include "metatypes/Instrument.h"  // for Instrument, EMPTY_INSTRUMENT

class QObject;

InstrumentsModel::InstrumentsModel(bool include_empty_input,
                                   QObject *parent_pointer_input)
    : QAbstractListModel(parent_pointer_input),
      include_empty(include_empty_input) {}

auto InstrumentsModel::data(const QModelIndex &index, int role) const
    -> QVariant {
  const auto &instruments = Instrument::get_all_instruments();
  auto row = index.row();
  const auto& instrument = include_empty ? (row == 0 ? Instrument::get_empty_instrument() : instruments.at(row - 1)) : instruments.at(row);
  if (role == Qt::DisplayRole) {
    return instrument.instrument_name;
  }
  if (role == Qt::UserRole) {
    return QVariant::fromValue(&instrument);
  }
  return {};
}

auto InstrumentsModel::rowCount(const QModelIndex & /*parent*/) const -> int {
  static auto number_of_instruments =
      static_cast<int>(Instrument::get_all_instruments().size());
  if (include_empty) {
    return number_of_instruments + 1;
  }
  return number_of_instruments;
}
