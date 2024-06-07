#include "src/InstrumentsModel.h"

#include <qabstractitemmodel.h>  // for QAbstractListModel, QModelIndex
#include <qnamespace.h>          // for DisplayRole, EditRole, ItemDataRole
#include <qstring.h>             // for QString
#include <qvariant.h>            // for QVariant

#include <cstddef>  // for size_t
#include <vector>   // for vector

#include "justly/Instrument.h"  // for Instrument

class QObject;

InstrumentsModel::InstrumentsModel(bool include_empty_input,
                                   QObject *parent_pointer_input)
    : QAbstractListModel(parent_pointer_input),
      include_empty(include_empty_input) {}

auto InstrumentsModel::data(const QModelIndex &index, int role) const
    -> QVariant {
  const auto &instruments = Instrument::get_all_instruments();
  auto row = index.row();
  const auto &instrument =
      include_empty ? (row == 0 ? Instrument::get_empty_instrument()
                                : instruments.at(static_cast<size_t>(row - 1)))
                    : instruments.at(static_cast<size_t>(row));
  auto data_role = static_cast<Qt::ItemDataRole>(role);
  if (data_role == Qt::DisplayRole) {
    return QString::fromStdString(instrument.instrument_name);
  }
  if (data_role == Qt::EditRole) {
    return QVariant::fromValue(&instrument);
  }
  return {};
}

auto InstrumentsModel::rowCount(const QModelIndex & /*parent*/) const -> int {
  static auto number_of_instruments =
      static_cast<int>(Instrument::get_all_instruments().size());
  return include_empty ? number_of_instruments + 1 : number_of_instruments;
}
