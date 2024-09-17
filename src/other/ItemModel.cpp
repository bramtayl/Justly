#include "other/ItemModel.hpp"

#include <QAbstractItemModel>
#include <QList>
#include <QObject>
#include <Qt>
#include <QtGlobal>

#include "other/other.hpp"

auto get_child_number(const QModelIndex &index) -> qsizetype {
  return to_qsizetype(index.row());
}

ItemModel::ItemModel(QObject *parent_pointer_input)
    : QAbstractTableModel(parent_pointer_input) {
}

void ItemModel::edited_cells(qsizetype first_child_number,
                                      qsizetype number_of_children,
                                      int left_column,
                                      int right_column) {
  emit dataChanged(
      index(first_child_number, left_column),
      index(first_child_number + number_of_children - 1, right_column),
      {Qt::DisplayRole, Qt::EditRole});
}

void ItemModel::begin_insert_rows(qsizetype first_child_number,
                                    qsizetype number_of_children) {
  beginInsertRows(QModelIndex(), static_cast<int>(first_child_number),
                  static_cast<int>(first_child_number + number_of_children) - 1);
}

void ItemModel::end_insert_rows() { endInsertRows(); }

void ItemModel::begin_remove_rows(qsizetype first_child_number,
                                    qsizetype number_of_children) {
  beginRemoveRows(QModelIndex(), static_cast<int>(first_child_number),
                  static_cast<int>(first_child_number + number_of_children) - 1);
}

void ItemModel::end_remove_rows() { endRemoveRows(); }

void ItemModel::begin_reset_model() {
  beginResetModel();
}

void ItemModel::end_reset_model() {
  endResetModel();
}
