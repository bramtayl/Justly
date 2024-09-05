#include "instrument/InstrumentsModel.hpp"

#include <QAbstractItemModel>
#include <QVariant>
#include <Qt>
#include <QList>

#include "instrument/Instrument.hpp"


class QObject;

InstrumentsModel::InstrumentsModel(QObject *parent_pointer_input)
    : QAbstractListModel(parent_pointer_input) {}

auto InstrumentsModel::rowCount(const QModelIndex & /*parent*/) const -> int {
  return static_cast<int>(get_all_instruments().size());
}

auto InstrumentsModel::data(const QModelIndex &index,
                            int role) const -> QVariant {
  auto row = index.row();
  const auto &instrument = get_all_instruments().at(row);
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    return QVariant::fromValue(&instrument);
  }
  return {};
}
