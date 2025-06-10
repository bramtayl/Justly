#pragma once 

#include "RowsModel.hpp"

template <RowInterface SubRow>
static void
insert_or_remove(RowsModel<SubRow> &rows_model, const int first_row_number,
                 const QList<SubRow> &new_rows, const int left_column,
                 const int right_column, const bool should_insert) {
  if (should_insert) {
    rows_model.insert_rows(first_row_number, new_rows, left_column,
                           right_column);
  } else {
    rows_model.remove_rows(first_row_number, new_rows.size());
  }
}

template <RowInterface SubRow> struct InsertRemoveRows : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int first_row_number;
  const QList<SubRow> new_rows;
  const int left_column;
  const int right_column;
  const bool backwards;
  InsertRemoveRows(RowsModel<SubRow> &rows_model_input,
                   const int first_row_number_input,
                   QList<SubRow> new_rows_input, const int left_column_input,
                   const int right_column_input, const bool backwards_input)
      : rows_model(rows_model_input), first_row_number(first_row_number_input),
        new_rows(std::move(new_rows_input)), left_column(left_column_input),
        right_column(right_column_input), backwards(backwards_input) {}

  void undo() override {
    insert_or_remove(rows_model, first_row_number, new_rows, left_column,
                     right_column, backwards);
  }

  void redo() override {
    insert_or_remove(rows_model, first_row_number, new_rows, left_column,
                     right_column, !backwards);
  }
};
