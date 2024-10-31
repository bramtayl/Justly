#pragma once

#include <QList>
#include <QUndoStack>
#include <QtGlobal>

#include "rows/RowsModel.hpp"

template <std::derived_from<Row> SubRow>
struct InsertRemoveRows : public QUndoCommand {
  RowsModel<SubRow>& rows_model;
  const int first_row_number;
  const QList<SubRow> new_rows;
  const bool backwards;
  InsertRemoveRows(RowsModel<SubRow>& rows_model_input,
                   int first_row_number_input, QList<SubRow> new_rows_input,
                   bool backwards_input)
      : rows_model(rows_model_input),
        first_row_number(first_row_number_input),
        new_rows(std::move(new_rows_input)), backwards(backwards_input) {
  };

  void undo() override { insert_or_remove(*this, backwards); };

  void redo() override { insert_or_remove(*this, !backwards); };
};

template <std::derived_from<Row> SubRow>
void insert_or_remove(const InsertRemoveRows<SubRow>& change, bool should_insert) {
  auto& rows_model = change.rows_model;
  auto& rows = get_rows(rows_model);
  const auto& new_rows = change.new_rows;
  const auto first_row_number = change.first_row_number;

  if (should_insert) {
    rows_model.begin_insert_rows(first_row_number, new_rows.size());
    std::copy(
        new_rows.cbegin(), new_rows.cend(),
        std::inserter(rows, rows.begin() + first_row_number));
    rows_model.end_insert_rows();
  } else {
    remove_rows(rows_model, first_row_number, new_rows.size());
  }
}
