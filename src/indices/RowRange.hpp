#pragma once

#include <cstddef>

class QItemSelectionRange;

struct RowRange {
  size_t first_child_number;
  size_t number_of_children;
  int parent_number;

  RowRange(size_t first_child_number_input,
                  size_t number_of_rows_input,
                  int parent_number);
  explicit RowRange(const QItemSelectionRange& range);
  
  [[nodiscard]] auto get_end_child_number() const -> size_t;
  [[nodiscard]] auto first_is_less(const RowRange& range_2) const -> bool;
  [[nodiscard]] auto is_chords() const -> bool;
  [[nodiscard]] auto get_parent_chord_number() const -> size_t;
};
