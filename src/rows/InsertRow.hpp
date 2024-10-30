#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "rows/RowsModel.hpp"

template <std::derived_from<Row> SubRow>
struct InsertRow : public QUndoCommand {
  RowsModel<SubRow>& rows_model;
  const int row_number;
  const SubRow new_row;

  InsertRow(RowsModel<SubRow>& rows_model_input, int row_number_input,
            QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        rows_model(rows_model_input),
        row_number(row_number_input), new_row(SubRow()) {
  };

  void undo() override { remove_rows(rows_model, row_number, 1); };
  void redo() override {
    rows_model.begin_insert_rows(row_number, 1);
    auto& rows = get_rows(rows_model);
    rows.insert(rows.begin() + row_number, new_row);
    rows_model.end_insert_rows();
  };
};
