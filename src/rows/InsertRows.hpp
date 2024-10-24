#pragma once

#include <QList>
#include <QUndoStack>
#include <QtGlobal>

#include "rows/RowsModel.hpp"

template <typename Row> struct InsertRows : public QUndoCommand {
  RowsModel<Row> *const rows_model_pointer;
  const int first_row_number;
  const QList<Row> new_rows;

  InsertRows(RowsModel<Row> *rows_model_pointer_input,
              int first_row_number_input, const QList<Row> &new_rows_input,
              QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        rows_model_pointer(rows_model_pointer_input),
        first_row_number(first_row_number_input), new_rows(new_rows_input) {
    Q_ASSERT(rows_model_pointer != nullptr);
  };

  void undo() override {
    rows_model_pointer->remove_rows(first_row_number, new_rows.size());
  };
  void redo() override {
    rows_model_pointer->insert_rows(first_row_number, new_rows);
  };
};
