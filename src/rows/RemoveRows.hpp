#pragma once

#include <QList>
#include <QUndoStack>
#include <QtGlobal>

#include "rows/RowsModel.hpp"

template <std::derived_from<Row> Row> struct RemoveRows : public QUndoCommand {
  RowsModel<Row> *const rows_model_pointer;
  const int first_row_number;
  const QList<Row> old_rows;

  RemoveRows(RowsModel<Row> *rows_model_pointer_input,
              int first_row_number_input, const QList<Row> &old_rows,
              QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        rows_model_pointer(rows_model_pointer_input),
        first_row_number(first_row_number_input), old_rows(old_rows) {
    Q_ASSERT(rows_model_pointer != nullptr);
  };

  void undo() override {
    rows_model_pointer->insert_rows(first_row_number, old_rows);
  };
  void redo() override {
    rows_model_pointer->remove_rows(first_row_number, old_rows.size());
  };
};
