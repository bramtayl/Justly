#pragma once

#include <QAbstractItemModel>
#include <QUndoStack>
#include <QtGlobal>
#include <utility>

#include "rows/Row.hpp"

template <std::derived_from<Row> SubRow> struct RowsModel;


template <std::derived_from<Row> SubRow> struct SetCell : public QUndoCommand {
  RowsModel<SubRow>& rows_model;
  QModelIndex index;
  const QVariant old_value;
  const QVariant new_value;

  explicit SetCell(RowsModel<SubRow>& rows_model_input,
                   const QModelIndex &index_input, QVariant new_value_input)
      : rows_model(rows_model_input), index(index_input),
        old_value(rows_model.data(index, Qt::DisplayRole)),
        new_value(std::move(new_value_input)){};

  void undo() override {
    set_model_data_directly(*this, false);
  };

  void redo() override {
    set_model_data_directly(*this, true);
  };
};

template <std::derived_from<Row> SubRow>
static void set_model_data_directly(SetCell<SubRow> &change, bool is_new) {
  const auto& index = change.index;
  auto& rows_model = change.rows_model;

  auto row_number = index.row();
  auto column = index.column();

  get_rows(rows_model)[row_number].set_data_directly(column, is_new ? change.new_value : change.old_value);
  rows_model.edited_cells(row_number, 1, column, column);
}