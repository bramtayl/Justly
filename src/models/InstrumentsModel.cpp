#include "models/InstrumentsModel.h"

#include <qabstractitemmodel.h>
#include <qnamespace.h>  // for DisplayRole
#include <qvariant.h>    // for QVariant

class QObject;

#include "metatypes/Instrument.h"

InstrumentsModel::InstrumentsModel(bool include_empty_input,
                                   QObject *parent_pointer_input)
    : QAbstractListModel(parent_pointer_input),
      instruments_pointer(&Instrument::get_all_instruments()),
      include_empty(include_empty_input) {}

auto InstrumentsModel::data(const QModelIndex &index, int role) const
    -> QVariant {
  if (role == Qt::DisplayRole) {
    auto row = index.row();
    if (include_empty) {
      if (row == 0) {
        return QVariant::fromValue(Instrument());
      }
      return QVariant::fromValue(instruments_pointer->at(row - 1));
    }
    return QVariant::fromValue(instruments_pointer->at(row));
  }
  return {};
}

auto InstrumentsModel::rowCount(const QModelIndex & /*parent*/) const -> int {
  auto number_of_instruments = static_cast<int>(instruments_pointer->size());
  if (include_empty) {
    return number_of_instruments + 1;
  }
  return number_of_instruments;
}
