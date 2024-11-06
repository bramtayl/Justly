#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "row/RowsModel.hpp"

template <std::derived_from<Row> SubRow>
struct InsertRow : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int row_number;
  const SubRow new_row;

  InsertRow(RowsModel<SubRow> &rows_model_input, int row_number_input)
      : rows_model(rows_model_input), row_number(row_number_input),
        new_row(SubRow()){};

  void undo() override { rows_model.remove_rows(row_number, 1); };
  void redo() override { rows_model.insert_row(row_number, new_row); };
};
