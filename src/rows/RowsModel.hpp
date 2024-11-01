#pragma once

#include <QAbstractItemModel>
#include <QString>
#include <QUndoStack>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include "rows/Row.hpp"
#include "rows/SetCell.hpp"

template <typename T> class QList;
class QObject;
class QUndoCommand;
class QUndoStack;

template <std::derived_from<Row> SubRow>
struct RowsModel : public QAbstractTableModel {
  QList<SubRow> *rows_pointer = nullptr;
  QUndoStack &undo_stack;

  explicit RowsModel(QUndoStack &undo_stack_input)
      : undo_stack(undo_stack_input){};

  [[nodiscard]] auto
  rowCount(const QModelIndex & /*parent_index*/) const -> int override {
    return static_cast<int>(get_const_rows(*this).size());
  }

  [[nodiscard]] auto
  columnCount(const QModelIndex & /*parent_index*/) const -> int override {
    return SubRow::get_number_of_columns();
  }

  [[nodiscard]] auto headerData(int section, Qt::Orientation orientation,
                                int role) const -> QVariant override {
    if (role != Qt::DisplayRole) {
      return {};
    }
    switch (orientation) {
    case Qt::Horizontal:
      return SubRow::get_column_name(section);
    case Qt::Vertical:
      return section + 1;
    }
  };

  [[nodiscard]] auto
  flags(const QModelIndex &index) const -> Qt::ItemFlags override {
    auto uneditable = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (is_column_editable(index.column())) {
      return uneditable | Qt::ItemIsEditable;
    }
    return uneditable;
  };

  [[nodiscard]] virtual auto
  is_column_editable(int /*column_number*/) const -> bool {
    return true;
  };

  [[nodiscard]] virtual auto get_status(int /*row_number*/) const -> QString {
    return "";
  };

  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override {
    Q_ASSERT(index.isValid());
    auto row_number = index.row();

    if (role == Qt::StatusTipRole) {
      return get_status(row_number);
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
      return {};
    }

    return get_const_rows(*this).at(row_number).get_data(index.column());
  }

  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             int role) -> bool override {
    // only set data for edit
    if (role != Qt::EditRole) {
      return false;
    };
    undo_stack.push(
        new SetCell<SubRow>( // NOLINT(cppcoreguidelines-owning-memory)
            *this, index, new_value));
    return true;
  };

  // unlock protected methods
  void edited_cells(int first_row_number, int number_of_rows, int left_column,
                    int right_column) {
    dataChanged(index(first_row_number, left_column),
                index(first_row_number + number_of_rows - 1, right_column),
                {Qt::DisplayRole, Qt::EditRole});
  };

  void begin_insert_rows(int first_row_number, int number_of_rows) {
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number + number_of_rows - 1);
  };

  void end_insert_rows() { endInsertRows(); };

  void begin_remove_rows(int first_row_number, int number_of_rows) {
    beginRemoveRows(QModelIndex(), first_row_number,
                    first_row_number + number_of_rows - 1);
  };

  void end_remove_rows() { endRemoveRows(); };

  void begin_reset_model() { beginResetModel(); }
  void end_reset_model() { endResetModel(); }
};

template <std::derived_from<Row> SubRow>
[[nodiscard]] auto get_rows(RowsModel<SubRow> &rows_model) -> QList<SubRow> & {
  auto *rows_pointer = rows_model.rows_pointer;
  Q_ASSERT(rows_pointer != nullptr);
  return *rows_pointer;
};

template <std::derived_from<Row> SubRow>
[[nodiscard]] auto get_const_rows(const RowsModel<SubRow> &rows_model) -> const QList<SubRow> & {
  const auto *rows_pointer = rows_model.rows_pointer;
  Q_ASSERT(rows_pointer != nullptr);
  return *rows_pointer;
};

template <std::derived_from<Row> SubRow>
void remove_rows(RowsModel<SubRow> &rows_model, int first_row_number,
                 int number_of_items) {
  auto &rows = get_rows(rows_model);

  rows_model.begin_remove_rows(first_row_number, number_of_items);
  rows.erase(rows.begin() + first_row_number,
             rows.begin() + first_row_number + number_of_items);
  rows_model.end_remove_rows();
}

template <std::derived_from<Row> SubRow>
[[nodiscard]] auto remove_rows_pointer(RowsModel<SubRow> &rows_model) {
  rows_model.begin_reset_model();
  rows_model.rows_pointer = nullptr;
  rows_model.end_reset_model();
}