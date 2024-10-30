#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "rows/RowsModel.hpp"

template <std::derived_from<Row> SubRow>
struct InsertRow : public QUndoCommand {
  RowsModel<SubRow> *const rows_model_pointer;
  const int row_number;
  const SubRow new_row;

  InsertRow(RowsModel<SubRow>& rows_model, int row_number_input,
            QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        rows_model_pointer(&rows_model),
        row_number(row_number_input), new_row(SubRow()) {
    Q_ASSERT(rows_model_pointer != nullptr);
  };

  void undo() override { rows_model_pointer->remove_rows(row_number, 1); };
  void redo() override { rows_model_pointer->insert_row(row_number, new_row); };
};
