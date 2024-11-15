#pragma once

#include <QAbstractItemModel>
#include <QList>
#include <QString>
#include <QUndoStack>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <utility>

#include "other/other.hpp"
#include "row/Row.hpp"

class QObject;
class QUndoCommand;
class QUndoStack;

template <std::derived_from<Row> SubRow> struct RowsModel;

template <std::derived_from<Row> SubRow> struct SetCell : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  QModelIndex index;
  const QVariant old_value;
  const QVariant new_value;

  explicit SetCell(RowsModel<SubRow> &rows_model_input,
                   const QModelIndex &index_input, QVariant new_value_input)
      : rows_model(rows_model_input), index(index_input),
        old_value(rows_model.data(index, Qt::DisplayRole)),
        new_value(std::move(new_value_input)){};

  void undo() override { set_model_data_directly(*this, old_value); };

  void redo() override { set_model_data_directly(*this, new_value); };
};

template <std::derived_from<Row> SubRow>
static void set_model_data_directly(SetCell<SubRow> &change,
                                    const QVariant &set_value) {
  const auto &index = change.index;
  change.rows_model.set_cell(index.row(), index.column(), set_value);
}

template <std::derived_from<Row> SubRow>
struct RowsModel : public QAbstractTableModel {
  QList<SubRow> *rows_pointer = nullptr;
  QUndoStack &undo_stack;

  explicit RowsModel(QUndoStack &undo_stack_input)
      : undo_stack(undo_stack_input){};

  [[nodiscard]] auto
  rowCount(const QModelIndex & /*parent_index*/) const -> int override {
    return static_cast<int>(get_const_reference(rows_pointer).size());
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
      return RowsModel::tr(SubRow::get_column_name(section));
    case Qt::Vertical:
      return section + 1;
    default:
      Q_ASSERT(false);
      return {};
    }
  };

  [[nodiscard]] auto
  flags(const QModelIndex &index) const -> Qt::ItemFlags override {
    auto uneditable = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (SubRow::is_column_editable(index.column())) {
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

    return get_const_reference(rows_pointer)
        .at(row_number)
        .get_data(index.column());
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

  // don't inline these functions because they use protected methods
  void set_cell(int row_number, int column_number, const QVariant &new_value) {
    get_reference(rows_pointer)[row_number].set_data(column_number, new_value);
    dataChanged(index(row_number, column_number),
                index(row_number, column_number),
                {Qt::DisplayRole, Qt::EditRole});
  }

  void set_cells(int first_row_number, const QList<SubRow> &new_rows,
                 int left_column, int right_column) {
    auto &rows = get_reference(rows_pointer);
    auto number_of_new_rows = new_rows.size();
    for (auto replace_number = 0; replace_number < number_of_new_rows;
         replace_number++) {
      rows[first_row_number + replace_number].copy_columns_from(
          new_rows.at(replace_number), left_column, right_column);
    }
    dataChanged(index(first_row_number, left_column),
                index(first_row_number + number_of_new_rows - 1, right_column),
                {Qt::DisplayRole, Qt::EditRole});
  }

  void insert_json_rows(int first_row_number, const nlohmann::json &json_rows) {
    auto &rows = get_reference(rows_pointer);
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number + static_cast<int>(json_rows.size()) - 1);
    json_to_rows(rows, json_rows);
    endInsertRows();
  }

  void insert_rows(int first_row_number, const QList<SubRow> &new_rows) {
    auto &rows = get_reference(rows_pointer);
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number + new_rows.size() - 1);
    std::copy(new_rows.cbegin(), new_rows.cend(),
              std::inserter(rows, rows.begin() + first_row_number));
    endInsertRows();
  }

  void insert_row(int row_number, const SubRow &new_row) {
    beginInsertRows(QModelIndex(), row_number, row_number);
    auto &rows = get_reference(rows_pointer);
    rows.insert(rows.begin() + row_number, new_row);
    endInsertRows();
  }

  void remove_rows(int first_row_number, int number_of_rows) {
    auto &rows = get_reference(rows_pointer);
    beginRemoveRows(QModelIndex(), first_row_number,
                    first_row_number + number_of_rows - 1);
    rows.erase(rows.begin() + first_row_number,
               rows.begin() + first_row_number + number_of_rows);
    endRemoveRows();
  }

  void set_rows_pointer(QList<SubRow> *new_rows = nullptr) {
    beginResetModel();
    rows_pointer = new_rows;
    endResetModel();
  }
};

template <std::derived_from<Row> SubRow> struct SetCells : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int first_row_number;
  const int left_column;
  const int right_column;
  const QList<SubRow> old_rows;
  const QList<SubRow> new_rows;
  explicit SetCells(RowsModel<SubRow> &rows_model_input,
                    int first_row_number_input, int left_column_input,
                    int right_column_input, QList<SubRow> old_rows_input,
                    QList<SubRow> new_rows_input)
      : rows_model(rows_model_input), first_row_number(first_row_number_input),
        left_column(left_column_input), right_column(right_column_input),
        old_rows(std::move(old_rows_input)),
        new_rows(std::move(new_rows_input)){};

  void undo() override { set_cells(*this, old_rows); };

  void redo() override { set_cells(*this, new_rows); }
};

template <std::derived_from<Row> SubRow>
static void set_cells(SetCells<SubRow> &change, const QList<SubRow> &set_rows) {
  change.rows_model.set_cells(change.first_row_number, set_rows,
                              change.left_column, change.right_column);
}

template <std::derived_from<Row> SubRow>
struct InsertRow : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int row_number;
  const SubRow new_row;

  InsertRow(RowsModel<SubRow> &rows_model_input, int row_number_input,
            SubRow new_row_input)
      : rows_model(rows_model_input), row_number(row_number_input),
        new_row(std::move(new_row_input)){};

  void undo() override { rows_model.remove_rows(row_number, 1); };
  void redo() override { rows_model.insert_row(row_number, new_row); };
};

template <std::derived_from<Row> SubRow>
struct InsertRemoveRows : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int first_row_number;
  const QList<SubRow> new_rows;
  const bool backwards;
  InsertRemoveRows(RowsModel<SubRow> &rows_model_input,
                   int first_row_number_input, QList<SubRow> new_rows_input,
                   bool backwards_input)
      : rows_model(rows_model_input), first_row_number(first_row_number_input),
        new_rows(std::move(new_rows_input)), backwards(backwards_input){};

  void undo() override { insert_or_remove(*this, backwards); };

  void redo() override { insert_or_remove(*this, !backwards); };
};

template <std::derived_from<Row> SubRow>
static void insert_or_remove(const InsertRemoveRows<SubRow> &change,
                             bool should_insert) {
  auto &rows_model = change.rows_model;
  const auto &new_rows = change.new_rows;
  const auto first_row_number = change.first_row_number;

  if (should_insert) {
    rows_model.insert_rows(first_row_number, new_rows);
  } else {
    rows_model.remove_rows(first_row_number, new_rows.size());
  }
}
