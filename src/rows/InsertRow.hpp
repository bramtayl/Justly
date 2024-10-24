#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "rows/RowsModel.hpp"

template <typename Row> struct InsertRow : public QUndoCommand {
  RowsModel<Row> *const rows_model_pointer;
  const int row_number;
  const Row new_row;

  InsertRow(RowsModel<Row> *rows_model_pointer_input, int row_number_input,
             const Row &new_row_input,
             QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        rows_model_pointer(rows_model_pointer_input),
        row_number(row_number_input), new_row(new_row_input) {
    Q_ASSERT(rows_model_pointer != nullptr);
  };

  void undo() override { rows_model_pointer->remove_rows(row_number, 1); };
  void redo() override {
    rows_model_pointer->insert_row(row_number, new_row);
  };
};
