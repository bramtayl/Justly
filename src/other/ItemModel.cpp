#include "other/ItemModel.hpp"

#include <QAbstractItemModel>
#include <QList>
#include <Qt>
#include <QtGlobal>

#include "other/other.hpp"

auto get_row_number(const QModelIndex &index) -> qsizetype {
  return to_qsizetype(index.row());
}

ItemModel::ItemModel(QObject *parent_pointer_input)
    : QAbstractTableModel(parent_pointer_input) {}

auto ItemModel::headerData(int section, Qt::Orientation orientation,
                           int role) const -> QVariant {
  if (role != Qt::DisplayRole) {
    return {};
  }
  switch (orientation) {
  case Qt::Horizontal:
    return get_column_name(section);
  case Qt::Vertical:
    return section + 1;
  }
}

auto ItemModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  auto uneditable = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  if (is_column_editable(index.column())) {
    return uneditable | Qt::ItemIsEditable;
  }
  return uneditable;
}

auto ItemModel::is_column_editable(int /*column_number*/) const -> bool {
  return true;
}

void ItemModel::edited_cells(qsizetype first_row_number,
                             qsizetype number_of_rows, int left_column,
                             int right_column) {
  dataChanged(
      index(first_row_number, left_column),
      index(first_row_number + number_of_rows - 1, right_column),
      {Qt::DisplayRole, Qt::EditRole});
}

void ItemModel::begin_insert_rows(qsizetype first_row_number,
                                  qsizetype number_of_rows) {
  beginInsertRows(QModelIndex(), static_cast<int>(first_row_number),
                  static_cast<int>(first_row_number + number_of_rows) -
                      1);
}

void ItemModel::end_insert_rows() { endInsertRows(); }

void ItemModel::begin_remove_rows(qsizetype first_row_number,
                                  qsizetype number_of_rows) {
  beginRemoveRows(QModelIndex(), static_cast<int>(first_row_number),
                  static_cast<int>(first_row_number + number_of_rows) -
                      1);
}

void ItemModel::end_remove_rows() { endRemoveRows(); }

void ItemModel::begin_reset_model() { beginResetModel(); }

void ItemModel::end_reset_model() { endResetModel(); }
