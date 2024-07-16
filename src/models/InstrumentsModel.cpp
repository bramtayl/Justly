#include "models/InstrumentsModel.hpp"

#include <QAbstractItemModel>
#include <QString>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <vector>

#include "justly/Instrument.hpp"
#include "other/private.hpp"

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

auto InstrumentsModel::data(const QModelIndex &index,
                            int role) const -> QVariant {
  auto row = index.row();
  const auto &all_instruments = get_all_instruments();
  Q_ASSERT(to_unsigned(row) < all_instruments.size());
  const auto &instrument = all_instruments[row];
  if (role == Qt::DisplayRole) {
    return QString::fromStdString(instrument.instrument_name);
  }
  if (role == Qt::EditRole) {
    return QVariant::fromValue(&instrument);
  }
  return {};
}
