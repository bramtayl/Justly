#include "percussion_set/PercussionSetsModel.hpp"

#include <QAbstractItemModel>
#include <QVariant>
#include <Qt>
#include <vector>

#include "other/templates.hpp"
#include "percussion_set/PercussionSet.hpp"

class QObject;

PercussionSetsModel::PercussionSetsModel(QObject *parent_pointer_input)
    : QAbstractListModel(parent_pointer_input) {}

auto PercussionSetsModel::rowCount(const QModelIndex & /*parent*/) const
    -> int {
  return static_cast<int>(get_all_percussion_sets().size());
}

auto PercussionSetsModel::data(const QModelIndex &index,
                               int role) const -> QVariant {
  auto row = index.row();
  const auto &instrument = get_const_item(get_all_percussion_sets(), row);
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    return QVariant::fromValue(&instrument);
  }
  return {};
}
