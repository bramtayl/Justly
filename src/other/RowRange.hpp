#pragma once

#include <cstddef>

class QItemSelection;
class QItemSelectionRange;

struct RowRange {
  size_t first_child_number;
  size_t number_of_children;
  int parent_number;

  RowRange(size_t first_child_number_input, size_t number_of_rows_input,
           int parent_number);
  explicit RowRange(const QItemSelectionRange &range);
};

[[nodiscard]] auto is_chords(const RowRange &row_range) -> bool;

[[nodiscard]] auto get_parent_chord_number(const RowRange &row_range) -> size_t;
[[nodiscard]] auto get_end_child_number(const RowRange &row_range) -> size_t;

[[nodiscard]] auto first_is_less(const RowRange &row_range_1,
                                 const RowRange &range_2) -> bool;

[[nodiscard]] auto
get_first_row_range(const QItemSelection &selection) -> RowRange;
