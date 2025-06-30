#pragma once

#include <QtWidgets/QAbstractItemView>

#include "rows/Row.hpp"

[[nodiscard]] static inline auto
get_number_of_rows(const QItemSelectionRange &range) {
  Q_ASSERT(range.isValid());
  return range.bottom() - range.top() + 1;
}

template <RowInterface SubRow> struct RowsModel : public QAbstractTableModel {
  QItemSelectionModel *selection_model_pointer = nullptr;

  [[nodiscard]] virtual auto is_valid() const -> bool { return true; };
  [[nodiscard]] virtual auto get_rows() const -> QList<SubRow> & = 0;

  [[nodiscard]] auto
  rowCount(const QModelIndex & /*parent_index*/) const -> int override {
    if (!is_valid()) {
      return 0;
    }
    return static_cast<int>(get_rows().size());
  }

  [[nodiscard]] auto
  columnCount(const QModelIndex & /*parent_index*/) const -> int override {
    return SubRow::get_number_of_columns();
  }

  [[nodiscard]] auto headerData(const int section,
                                const Qt::Orientation orientation,
                                const int role) const -> QVariant override {
    if (role != Qt::DisplayRole) {
      return {};
    }
    switch (orientation) {
    case Qt::Horizontal:
      return SubRow::get_column_name(section);
    case Qt::Vertical:
      return section + 1;
    default:
      Q_ASSERT(false);
      return {};
    }
  }

  [[nodiscard]] auto
  flags(const QModelIndex &index) const -> Qt::ItemFlags override {
    const auto uneditable = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    return SubRow::is_column_editable(index.column())
               ? (uneditable | Qt::ItemIsEditable)
               : uneditable;
  }

  [[nodiscard]] virtual auto
  get_status(const int /*row_number*/) const -> QString {
    return "";
  }

  [[nodiscard]] auto data(const QModelIndex &index,
                          const int role) const -> QVariant override {
    Q_ASSERT(index.isValid());
    const auto row_number = index.row();

    if (role == Qt::StatusTipRole) {
      return get_status(row_number);
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
      return get_rows().at(row_number).get_data(index.column());
    }

    return {};
  }

  // don't inline these functions because they use protected methods
  void set_cell(const QModelIndex &set_index, const QVariant &new_value) {
    const auto row_number = set_index.row();
    const auto column_number = set_index.column();

    get_rows()[row_number].set_data(column_number, new_value);
    dataChanged(set_index, set_index);
    get_reference(selection_model_pointer)
        .select(set_index,
                QItemSelectionModel::Select | QItemSelectionModel::Clear);
  }

  void set_cells(const QItemSelectionRange &range,
                 const QList<SubRow> &new_rows) {
    Q_ASSERT(range.isValid());

    auto &rows = get_rows();
    const auto number_of_new_rows = new_rows.size();

    const auto &top_left_index = range.topLeft();
    const auto &bottom_right_index = range.bottomRight();

    const auto first_row_number = range.top();
    const auto left_column = range.left();
    const auto right_column = range.right();

    for (auto replace_number = 0; replace_number < number_of_new_rows;
         replace_number++) {
      auto &row = rows[first_row_number + replace_number];
      const auto &new_row = new_rows.at(replace_number);
      for (auto column_number = left_column; column_number <= right_column;
           column_number++) {
        row.copy_column_from(new_row, column_number);
      }
    }
    dataChanged(top_left_index, bottom_right_index);
    get_reference(selection_model_pointer)
        .select(QItemSelection(top_left_index, bottom_right_index),
                QItemSelectionModel::Select | QItemSelectionModel::Clear);
  }

  void delete_cells(const QItemSelectionRange &range) {
    Q_ASSERT(range.isValid());

    const auto &top_left_index = range.topLeft();
    const auto &bottom_right_index = range.bottomRight();

    const auto first_row_number = range.top();
    const auto left_column = range.left();
    const auto right_column = range.right();
    const auto number_of_rows = get_number_of_rows(range);

    auto &rows = get_rows();
    for (auto replace_number = 0; replace_number < number_of_rows;
         replace_number++) {
      auto &row = rows[first_row_number + replace_number];
      const SubRow empty_row;
      for (auto column_number = left_column; column_number <= right_column;
           column_number++) {
        row.copy_column_from(empty_row, column_number);
      }
    }
    dataChanged(top_left_index, bottom_right_index);
    get_reference(selection_model_pointer)
        .select(QItemSelection(top_left_index, bottom_right_index),
                QItemSelectionModel::Select | QItemSelectionModel::Clear);
  }

  void insert_xml_rows(const int first_row_number, xmlNode &rows_node) {
    auto count = 0;
    xmlNode *xml_row_pointer = xmlFirstElementChild(&rows_node);
    while (xml_row_pointer != nullptr) {
      count++;
      xml_row_pointer = xmlNextElementSibling(xml_row_pointer);
    }
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number + count - 1);
    xml_to_rows(get_rows(), rows_node);
    endInsertRows();
  }

  void insert_rows(const int first_row_number, const QList<SubRow> &new_rows,
                   const int left_column, const int right_column) {
    auto &rows = get_rows();
    const auto number_of_rows = static_cast<int>(new_rows.size());
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number + number_of_rows - 1);
    std::copy(new_rows.cbegin(), new_rows.cend(),
              std::inserter(rows, rows.begin() + first_row_number));
    endInsertRows();
    get_reference(selection_model_pointer)
        .select(QItemSelection(
                    index(first_row_number, left_column),
                    index(first_row_number + number_of_rows - 1, right_column)),
                QItemSelectionModel::Select | QItemSelectionModel::Clear);
  }

  void insert_row(const int row_number, SubRow new_row) {
    beginInsertRows(QModelIndex(), row_number, row_number);
    auto &rows = get_rows();
    rows.insert(rows.begin() + row_number, std::move(new_row));
    endInsertRows();
    get_reference(selection_model_pointer)
        .select(index(row_number, 0), QItemSelectionModel::Select |
                                          QItemSelectionModel::Clear |
                                          QItemSelectionModel::Rows);
  }

  void remove_rows(const int first_row_number, int number_of_rows) {
    auto &rows = get_rows();
    beginRemoveRows(QModelIndex(), first_row_number,
                    first_row_number + number_of_rows - 1);
    rows.erase(rows.begin() + first_row_number,
               rows.begin() + first_row_number + number_of_rows);
    endRemoveRows();
  }
};

[[nodiscard]] static inline auto make_range(QAbstractItemModel &model,
                                            const int first_row_number,
                                            const int number_of_rows,
                                            const int left_column,
                                            const int right_column) {
  return QItemSelectionRange(
      model.index(first_row_number, left_column),
      model.index(first_row_number + number_of_rows - 1, right_column));
}