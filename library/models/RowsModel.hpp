#pragma once

#include <QtCore/QItemSelectionRange>

#include "rows/Row.hpp"

struct Song;

template <RowInterface SubRow>
struct RowsModel : public QAbstractTableModel {
  Song& song;
  QItemSelectionModel* selection_model_pointer = nullptr;
  QList<SubRow>* rows_pointer = nullptr;
  int parent_chord_number = -1;

  explicit RowsModel(Song& song_input) : song(song_input) {}

  [[nodiscard]] auto is_valid() const -> bool {
    return rows_pointer != nullptr;
  };

  [[nodiscard]] auto get_rows() const -> QList<SubRow>& {
    return get_reference(rows_pointer);
  };

  void set_rows_pointer(QList<SubRow>* const new_rows_pointer = nullptr,
                        const int new_parent_chord_number = -1) {
    QAbstractTableModel::beginResetModel();
    rows_pointer = new_rows_pointer;
    parent_chord_number = new_parent_chord_number;
    QAbstractTableModel::endResetModel();
  }

  [[nodiscard]] auto rowCount(const QModelIndex& /*parent_index*/) const
      -> int override {
    if (!is_valid()) {
      return 0;
    }
    return static_cast<int>(get_rows().size());
  }

  [[nodiscard]] auto columnCount(const QModelIndex& /*parent_index*/) const
      -> int override {
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
    }
    Q_UNREACHABLE();
  }

  [[nodiscard]] auto flags(const QModelIndex& index) const
      -> Qt::ItemFlags override {
    const auto uneditable = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    return column_is_editable<SubRow>(index.column())
               ? (uneditable | Qt::ItemIsEditable)
               : uneditable;
  }

  virtual void add_to_status(QTextStream& /*stream*/, const int /*row_number*/,
                             const SubRow& /*row*/) const {}

  [[nodiscard]] virtual auto get_display_data(const int row_number,
                                              const int column_number) const
      -> QVariant {
    return get_rows().at(row_number).get_data(column_number);
  }

  [[nodiscard]] auto data(const QModelIndex& index, const int role) const
      -> QVariant override {
    Q_ASSERT(index.isValid());
    const auto row_number = index.row();

    if (role == Qt::StatusTipRole) {
      QString result;
      QTextStream stream(&result);
      const auto& row = get_rows().at(row_number);
      add_to_status(stream, row_number, row);
      return result;
    }

    if (role == Qt::DisplayRole) {
      return get_display_data(row_number, index.column());
    }

    if (role == Qt::EditRole) {
      return get_rows().at(row_number).get_data(index.column());
    }

    return {};
  }

  // what a new row, or a deleted cell, starts as; notes override this so
  // they always name an existing voice
  [[nodiscard]] virtual auto make_empty_row() const -> SubRow {
    return SubRow();
  }

  [[nodiscard]] virtual auto check_cell(const int /*column_number*/,
                                        const QVariant& /*new_value*/) const
      -> bool {
    return true;
  }

  // replaces the selection with range, e.g. the cells an edit just changed
  void select_only(const QItemSelectionRange& range) const {
    get_reference(selection_model_pointer)
        .select(QItemSelection(range.topLeft(), range.bottomRight()),
                QItemSelectionModel::Select | QItemSelectionModel::Clear);
  }

  // don't inline these functions because they use protected methods
  void set_cell(const int row_number, const int column_number,
                const QVariant& new_value) {
    const auto set_index = index(row_number, column_number);

    get_rows()[row_number].set_data(column_number, new_value);
    dataChanged(set_index, set_index);
    select_only(QItemSelectionRange(set_index));
  }

  void set_cells(const QItemSelectionRange& range,
                 const QList<SubRow>& new_rows) {
    Q_ASSERT(range.isValid());

    auto& rows = get_rows();
    const auto number_of_new_rows = new_rows.size();

    const auto first_row_number = range.top();
    const auto left_column = range.left();
    const auto right_column = range.right();

    for (auto replace_number = 0; replace_number < number_of_new_rows;
         replace_number++) {
      auto& row = rows[first_row_number + replace_number];
      const auto& new_row = new_rows.at(replace_number);
      for (auto column_number = left_column; column_number <= right_column;
           column_number++) {
        copy_column(row, new_row, column_number);
      }
    }
    dataChanged(range.topLeft(), range.bottomRight());
    select_only(range);
  }

  // swaps in every row at once, e.g. when loading a file
  void replace_all_rows(QList<SubRow> new_rows) {
    beginResetModel();
    get_rows() = std::move(new_rows);
    endResetModel();
  }

  void insert_rows(const int first_row_number, const QList<SubRow>& new_rows,
                   const int left_column, const int right_column) {
    auto& rows = get_rows();
    const auto number_of_rows = static_cast<int>(new_rows.size());
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number + number_of_rows - 1);
    std::copy(new_rows.cbegin(), new_rows.cend(),
              std::inserter(rows, rows.begin() + first_row_number));
    endInsertRows();
    select_only(make_range(*this, first_row_number, number_of_rows,
                           left_column, right_column));
  }

  void remove_rows(const int first_row_number, int number_of_rows) {
    auto& rows = get_rows();
    beginRemoveRows(QModelIndex(), first_row_number,
                    first_row_number + number_of_rows - 1);
    rows.erase(rows.begin() + first_row_number,
               rows.begin() + first_row_number + number_of_rows);
    endRemoveRows();
  }
};