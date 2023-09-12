#include "InstrumentsModel.h"

#include <qnamespace.h>  // for DisplayRole

class QObject;

#include "Instrument.h"

InstrumentsModel::InstrumentsModel(bool include_empty_input,
                                   QObject *parent_pointer_input)
    : instrument_pointers(Instrument::get_all_instrument_pointers()),
      include_empty(include_empty_input),
      QAbstractListModel(parent_pointer_input) {}

auto InstrumentsModel::data(const QModelIndex &index, int role) const
    -> QVariant {
  if (role == Qt::DisplayRole) {
    auto row = index.row();
    if (include_empty) {
      if (row == 0) {
        return "";
      }
      return instrument_pointers[row - 1] -> instrument_name;
    }
    return instrument_pointers[row] -> instrument_name;
  }
  return {};
};

auto InstrumentsModel::rowCount(const QModelIndex & /*parent*/) const -> int {
  auto number_of_instruments = static_cast<int>(instrument_pointers.size());
  if (include_empty) {
    return number_of_instruments + 1;
  }
  return number_of_instruments;
};