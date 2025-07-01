#pragma once

#include <QtGui/QUndoCommand>

#include "models/RowsModel.hpp"

template <RowInterface SubRow> struct DeleteCells : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int first_row_number;
  const int number_of_rows;
  const int left_column;
  const int right_column;
  const QList<SubRow> old_rows;

  explicit DeleteCells(RowsModel<SubRow> &rows_model_input,
                       const QItemSelectionRange &range_input)
      : rows_model(rows_model_input), first_row_number(range_input.top()),
        number_of_rows(get_number_of_rows(range_input)),
        left_column(range_input.left()), right_column(range_input.right()),
        old_rows(copy_items(rows_model.get_rows(), first_row_number,
                            number_of_rows)) {}

  void undo() override {
    rows_model.set_cells(make_range(rows_model, first_row_number,
                                    number_of_rows, left_column, right_column),
                         old_rows);
  }

  void redo() override {
    rows_model.delete_cells(make_range(rows_model, first_row_number,
                                       number_of_rows, left_column,
                                       right_column));
  }
};
