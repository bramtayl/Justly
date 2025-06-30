#pragma once

#include <QtGui/QUndoStack>

#include "models/RowsModel.hpp"

template <RowInterface SubRow> struct SetCells : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int first_row_number;
  const int number_of_rows;
  const int left_column;
  const int right_column;
  const QList<SubRow> old_rows;
  const QList<SubRow> new_rows;

  explicit SetCells(RowsModel<SubRow> &rows_model_input,
                    const int first_row_number_input,
                    const int number_of_rows_input, const int left_column_input,
                    const int right_column_input, QList<SubRow> new_rows_input)
      : rows_model(rows_model_input), first_row_number(first_row_number_input),
        number_of_rows(number_of_rows_input), left_column(left_column_input),
        right_column(right_column_input),
        old_rows(copy_items(rows_model.get_rows(), first_row_number,
                            number_of_rows)),
        new_rows(std::move(new_rows_input)) {}

  void undo() override {
    rows_model.set_cells(make_range(rows_model, first_row_number,
                                    number_of_rows, left_column, right_column),
                         old_rows);
  }

  void redo() override {
    rows_model.set_cells(make_range(rows_model, first_row_number,
                                    number_of_rows, left_column, right_column),
                         new_rows);
  }
};
