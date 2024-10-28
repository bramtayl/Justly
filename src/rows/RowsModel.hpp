#pragma once

#include <QAbstractItemModel>
#include <QString>
#include <QUndoStack>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <iterator>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include "rows/Row.hpp"
#include "rows/SetCell.hpp"

template <typename T> class QList;
class QObject;
class QUndoCommand;
class QUndoStack;

// A row should have the following methods:
// Row(const nlohmann::json &json_chord);

// [[nodiscard]] auto get_data(int column_number) const -> QVariant;
// void set_data_directly(int column, const QVariant &new_value);

// void copy_columns_from(const Row &template_row, int left_column,
//                        int right_column);
// [[nodiscard]] auto to_json(int left_column,
//                            int right_column) const -> nlohmann::json;
template <std::derived_from<Row> SubRow>
struct RowsModel : public QAbstractTableModel {
  QList<SubRow> *rows_pointer = nullptr;
  QUndoStack *const undo_stack_pointer;

  explicit RowsModel(QUndoStack *undo_stack_pointer_input,
                     QList<SubRow> *rows_pointer_input = nullptr,
                     QObject *parent_pointer_input = nullptr)
      : QAbstractTableModel(parent_pointer_input),
        rows_pointer(rows_pointer_input),
        undo_stack_pointer(undo_stack_pointer_input){};

  [[nodiscard]] auto set_rows_pointer(QList<SubRow> *new_rows_pointer) {
    beginResetModel();
    rows_pointer = new_rows_pointer;
    endResetModel();
  }

  [[nodiscard]] auto
  rowCount(const QModelIndex & /*parent_index*/) const -> int override {
    Q_ASSERT(rows_pointer != nullptr);
    return static_cast<int>(rows_pointer->size());
  }

  [[nodiscard]] auto headerData(int section, Qt::Orientation orientation,
                                int role) const -> QVariant override {
    if (role != Qt::DisplayRole) {
      return {};
    }
    switch (orientation) {
    case Qt::Horizontal:
      return get_column_name(section);
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
  get_column_name(int column_number) const -> QString = 0;

  [[nodiscard]] virtual auto
  is_column_editable(int /*column_number*/) const -> bool {
    return true;
  };

  [[nodiscard]] virtual auto get_status(int row_number) const -> QString = 0;

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

    Q_ASSERT(rows_pointer != nullptr);
    return rows_pointer->at(row_number).get_data(index.column());
  }

  void set_data_directly(const QModelIndex &index, const QVariant &new_value) {
    Q_ASSERT(rows_pointer != nullptr);
    auto row_number = index.row();
    auto column = index.column();

    auto &row = (*rows_pointer)[row_number];
    row.set_data_directly(column, new_value);
    edited_cells(row_number, 1, column, column);
  }

  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             int role) -> bool override {
    // only set data for edit
    if (role != Qt::EditRole) {
      return false;
    };
    undo_stack_pointer->push(
        new SetCell<SubRow>( // NOLINT(cppcoreguidelines-owning-memory)
            this, index, new_value));
    return true;
  };

  auto get_item(int row_number) -> const SubRow & {
    Q_ASSERT(rows_pointer != nullptr);
    return rows_pointer->at(row_number);
  }

  // internal functions
  void edited_cells(int first_row_number, int number_of_rows, int left_column,
                    int right_column) {
    dataChanged(index(first_row_number, left_column),
                index(first_row_number + number_of_rows - 1, right_column),
                {Qt::DisplayRole, Qt::EditRole});
  };

  void set_cells(int first_row_number, int left_column, int right_column,
                 const QList<SubRow> &template_items) {
    Q_ASSERT(rows_pointer != nullptr);
    auto number_of_items = template_items.size();
    for (auto replace_number = 0; replace_number < number_of_items;
         replace_number++) {
      (*rows_pointer)[first_row_number + replace_number].copy_columns_from(
          template_items.at(replace_number), left_column, right_column);
    }
    edited_cells(first_row_number, static_cast<int>(number_of_items),
                 left_column, right_column);
  }

  void begin_insert_rows(int first_row_number, int number_of_rows) {
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number + number_of_rows - 1);
  };

  void end_insert_rows() { endInsertRows(); };

  void insert_rows(int first_row_number, const QList<SubRow> &new_rows) {
    Q_ASSERT(rows_pointer != nullptr);

    begin_insert_rows(first_row_number, new_rows.size());
    std::copy(
        new_rows.cbegin(), new_rows.cend(),
        std::inserter(*rows_pointer, rows_pointer->begin() + first_row_number));
    end_insert_rows();
  };

  void insert_row(int row_number, const SubRow &new_row) {
    Q_ASSERT(rows_pointer != nullptr);

    begin_insert_rows(row_number, 1);
    rows_pointer->insert(rows_pointer->begin() + row_number, new_row);
    end_insert_rows();
  }

  void remove_rows(int first_row_number, int number_of_items) {
    Q_ASSERT(rows_pointer != nullptr);

    beginRemoveRows(QModelIndex(), first_row_number,
                    first_row_number + number_of_items - 1);
    rows_pointer->erase(rows_pointer->begin() + first_row_number,
                        rows_pointer->begin() + first_row_number +
                            number_of_items);
    endRemoveRows();
  }
};
