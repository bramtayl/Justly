#pragma once

#include "rows/Row.hpp"

template <RowInterface SubRow> struct Cells {
  const int left_column;
  const int right_column;
  const QList<SubRow> rows;
  Cells(const int left_column_input, const int right_column_input,
        QList<SubRow> rows_input)
      : left_column(left_column_input), right_column(right_column_input),
        rows(std::move(rows_input)) {}
};