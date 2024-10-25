#pragma once

#include <QList>
#include <QUndoStack>
#include <QtGlobal>

#include "rows/RowsModel.hpp"

template <std::derived_from<Row> SubRow> struct InsertRemoveRows : public QUndoCommand {
  RowsModel<SubRow> *const rows_model_pointer;
  const int first_row_number;
  const QList<SubRow> new_rows;
  const bool backwards;
  InsertRemoveRows(RowsModel<SubRow> *rows_model_pointer_input,
              int first_row_number_input, const QList<SubRow> &new_rows_input, bool backwards_input,
              QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        rows_model_pointer(rows_model_pointer_input),
        first_row_number(first_row_number_input), new_rows(new_rows_input), backwards(backwards_input) {
    Q_ASSERT(rows_model_pointer != nullptr);
  };

  void insert_or_remove(bool should_insert) const {
    if (should_insert) {
      rows_model_pointer->insert_rows(first_row_number, new_rows);
    } else {
      rows_model_pointer->remove_rows(first_row_number, new_rows.size());
    }
  }

  void undo() override {
    insert_or_remove(backwards);    
  };
  
  void redo() override {
    insert_or_remove(!backwards);
  };
};
