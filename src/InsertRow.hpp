#pragma once

#include "RowsModel.hpp"

template <RowInterface SubRow> struct InsertRow : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int row_number;
  const SubRow new_row;

  InsertRow(RowsModel<SubRow> &rows_model_input, const int row_number_input,
            SubRow new_row_input = SubRow())
      : rows_model(rows_model_input), row_number(row_number_input),
        new_row(std::move(new_row_input)) {}

  void undo() override { rows_model.remove_rows(row_number, 1); }

  void redo() override { rows_model.insert_row(row_number, new_row); }
};
