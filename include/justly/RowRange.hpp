#pragma once

#include <cstddef>

struct RowRange {
  size_t first_child_number;
  size_t number_of_rows;
  int parent_number;
  RowRange(size_t first_child_number_input,
                  size_t number_of_rows_input,
                  int parent_number);
  auto operator<(const RowRange& range_2) const -> bool;
};