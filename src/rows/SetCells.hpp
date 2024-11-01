#pragma once

#include <QList>
#include <QUndoStack>
#include <QtGlobal>

#include "rows/RowsModel.hpp"

template <std::derived_from<Row> SubRow> struct SetCells : public QUndoCommand {
  RowsModel<SubRow>& rows_model;
  const int first_row_number;
  const int left_column;
  const int right_column;
  const QList<SubRow> old_rows;
  const QList<SubRow> new_rows;
  explicit SetCells(RowsModel<SubRow>& rows_model_input,
                    int first_row_number_input, int left_column_input,
                    int right_column_input, QList<SubRow> old_rows_input,
                    QList<SubRow> new_rows_input)
      : rows_model(rows_model_input),
        first_row_number(first_row_number_input),
        left_column(left_column_input), right_column(right_column_input),
        old_rows(std::move(old_rows_input)),
        new_rows(std::move(new_rows_input)) {
  };

  void undo() override {
    set_cells(*this, false);
  };

  void redo() override {
    set_cells(*this, true);
  }
};

template <std::derived_from<Row> SubRow>
static void set_cells(SetCells<SubRow> &change, bool is_new) {
  auto& rows_model = change.rows_model;
  const auto left_column = change.left_column;
  const auto right_column = change.right_column;
  const auto first_row_number = change.first_row_number;

  const auto& template_rows = is_new ? change.new_rows : change.old_rows;
  auto &rows = get_rows(rows_model);
  auto number_of_items = template_rows.size();
  for (auto replace_number = 0; replace_number < number_of_items;
       replace_number++) {
    rows[first_row_number + replace_number].copy_columns_from(
        template_rows.at(replace_number), left_column, right_column);
  }
  rows_model.edited_cells(first_row_number, static_cast<int>(number_of_items),
                          left_column, right_column);
}
