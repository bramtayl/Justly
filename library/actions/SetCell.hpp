#pragma once

#include <QtCore/QModelIndex>
#include <QtGui/QUndoCommand>

#include "rows/Row.hpp"

template <RowInterface SubRow>
struct RowsModel;

template <RowInterface SubRow>
struct SetCell : public QUndoCommand {
  RowsModel<SubRow>& rows_model;
  const int row_number;
  const int column_number;
  const QVariant old_value;
  const QVariant new_value;

  explicit SetCell(RowsModel<SubRow>& rows_model_input,
                   const QModelIndex& index, QVariant new_value_input)
      : rows_model(rows_model_input), row_number(index.row()),
        column_number(index.column()), old_value(index.data(Qt::EditRole)),
        new_value(std::move(new_value_input)) {}

  void undo() override {
    rows_model.set_cell(row_number, column_number, old_value);
  }

  void redo() override {
    rows_model.set_cell(row_number, column_number, new_value);
  }
};
