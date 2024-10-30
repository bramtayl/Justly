#pragma once

#include <QList>
#include <QUndoStack>
#include <QtGlobal>

#include "rows/RowsModel.hpp"

template <std::derived_from<Row> SubRow>
struct InsertRemoveRows : public QUndoCommand {
  RowsModel<SubRow> *const rows_model_pointer;
  const int first_row_number;
  const QList<SubRow> new_rows;
  const bool backwards;
  InsertRemoveRows(RowsModel<SubRow>& rows_model,
                   int first_row_number_input, QList<SubRow> new_rows_input,
                   bool backwards_input,
                   QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        rows_model_pointer(&rows_model),
        first_row_number(first_row_number_input),
        new_rows(std::move(new_rows_input)), backwards(backwards_input) {
    Q_ASSERT(rows_model_pointer != nullptr);
  };

  void undo() override { insert_or_remove(*this, backwards); };

  void redo() override { insert_or_remove(*this, !backwards); };
};

template <std::derived_from<Row> SubRow>
void insert_or_remove(const InsertRemoveRows<SubRow>& change, bool should_insert) {
  auto& rows_model = *change.rows_model_pointer;
  const auto& new_rows = change.new_rows;
  const auto first_row_number = change.first_row_number;

  if (should_insert) {
    rows_model.insert_rows(first_row_number, new_rows);
  } else {
    rows_model.remove_rows(first_row_number, new_rows.size());
  }
}
