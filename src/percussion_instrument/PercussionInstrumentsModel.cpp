#include "percussion_instrument/PercussionInstrumentsModel.hpp"

#include <QAbstractItemModel>
#include <QList>
#include <QVariant>
#include <Qt>

#include "percussion_instrument/PercussionInstrument.hpp"

class QObject;

PercussionInstrumentsModel::PercussionInstrumentsModel(
    QObject *parent_pointer_input)
    : QAbstractListModel(parent_pointer_input) {}

auto PercussionInstrumentsModel::rowCount(const QModelIndex & /*parent*/) const
    -> int {
  return static_cast<int>(get_all_percussion_instruments().size());
}

auto PercussionInstrumentsModel::data(const QModelIndex &index,
                                      int role) const -> QVariant {
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    return QVariant::fromValue(&get_all_percussion_instruments().at(index.row()));
  }
  return {};
}
