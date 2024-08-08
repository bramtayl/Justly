#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QtGlobal>
#include <algorithm>
#include <cstddef>

#include "indices/RowRange.hpp"
#include "other/conversions.hpp"

RowRange::RowRange(size_t first_child_number_input, size_t number_of_rows_input,
                   int parent_number_input)
    : first_child_number(first_child_number_input),
      number_of_children(number_of_rows_input),
      parent_number(parent_number_input) {}

RowRange::RowRange(const QItemSelectionRange &range)
    : first_child_number(to_size_t(range.top())),
      number_of_children(to_size_t(range.bottom() - range.top() + 1)),
      parent_number(range.parent().row()) {}

auto is_chords(const RowRange &row_range) -> bool {
  return row_range.parent_number == -1;
}

auto get_parent_chord_number(const RowRange &row_range) -> size_t {
  return to_size_t(row_range.parent_number);
}

auto get_end_child_number(const RowRange &row_range) -> size_t {
  return row_range.first_child_number + row_range.number_of_children;
}

auto first_is_less(const RowRange &range_1, const RowRange &range_2) -> bool {
  auto range_2_is_chords = is_chords(range_2);
  if (is_chords(range_1)) {
    auto range_1_chord_number = range_1.first_child_number;
    if (range_2_is_chords) {
      return range_1_chord_number < range_2.first_child_number;
    }
    // chord is less than note 2 if they are tied
    return range_1_chord_number <= get_parent_chord_number(range_2);
  }
  auto range_1_chord_number = get_parent_chord_number(range_1);
  if (range_2_is_chords) {
    // note isn't less than chord 2 if they are tied
    return range_1_chord_number < range_2.first_child_number;
  }
  return range_1_chord_number < get_parent_chord_number(range_2);
}

auto get_first_row_range(const QItemSelection &selection) -> RowRange {
  auto min_pointer = std::min_element(selection.begin(), selection.end(),
                                      [](const QItemSelectionRange &range_1,
                                         const QItemSelectionRange &range_2) {
                                        return first_is_less(RowRange(range_1), 
                                            RowRange(range_2));
                                      });
  Q_ASSERT(min_pointer != nullptr);
  return RowRange(*min_pointer);
}
