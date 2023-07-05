#include "InstrumentsModel.h"

#include <qnamespace.h>  // for DisplayRole

#include <memory>  // for allocator_traits<>::value_type

class QObject;
#include "Instrument.h"

InstrumentsModel::InstrumentsModel(
    const std::vector<Instrument> &instruments_input, bool include_empty_input,
    QObject *parent_pointer_input)
    : instruments(instruments_input),
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
      return instruments[row - 1].name;
    }
    return instruments[row].name;
  }
  return {};
};

auto InstrumentsModel::rowCount(const QModelIndex & /*parent*/) const -> int {
  auto number_of_instruments = static_cast<int>(instruments.size());
  if (include_empty) {
    return number_of_instruments + 1;
  }
  return number_of_instruments;
};