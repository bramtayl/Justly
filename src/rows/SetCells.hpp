#pragma once

#include <QList>
#include <QUndoStack>
#include <QtGlobal>

#include "rows/RowsModel.hpp"

template <std::derived_from<Row> SubRow> struct SetCells : public QUndoCommand {
  RowsModel<SubRow> *const rows_model_pointer;
  const int first_row_number;
  const int left_column;
  const int right_column;
  const QList<SubRow> old_rows;
  const QList<SubRow> new_rows;
  explicit SetCells(RowsModel<SubRow> *rows_model_pointer_input,
                    int first_row_number_input, int left_column_input,
                    int right_column_input, QList<SubRow> old_rows_input,
                    QList<SubRow> new_rows_input,
                    QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        rows_model_pointer(rows_model_pointer_input),
        first_row_number(first_row_number_input),
        left_column(left_column_input), right_column(right_column_input),
        old_rows(std::move(old_rows_input)), new_rows(std::move(new_rows_input)) {
    Q_ASSERT(rows_model_pointer != nullptr);
  };

  void undo() override {
    rows_model_pointer->set_cells(first_row_number, left_column, right_column,
                                   old_rows);
  };

  void redo() override {
    rows_model_pointer->set_cells(first_row_number, left_column, right_column,
                                   new_rows);
  }
};
