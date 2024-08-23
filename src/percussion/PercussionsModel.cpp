#include "percussion/PercussionsModel.hpp"

#include <QAbstractItemModel>
#include <QVariant>
#include <Qt>
#include <vector>

#include "percussion/Percussion.hpp"
#include "other/templates.hpp"

PercussionsModel::PercussionsModel(QObject *parent_pointer_input)
    : QAbstractListModel(parent_pointer_input) {}

auto PercussionsModel::rowCount(const QModelIndex & /*parent*/) const -> int {
  return static_cast<int>(get_all_percussions().size());
}

auto PercussionsModel::flags(const QModelIndex & /*index*/) const -> Qt::ItemFlags {
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

auto PercussionsModel::data(const QModelIndex &index,
                            int role) const -> QVariant {
  auto row = index.row();
  const auto &instrument = get_const_item(get_all_percussions(), row);
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    return QVariant::fromValue(&instrument);
  }
  return {};
}
