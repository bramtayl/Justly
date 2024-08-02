#include <QAbstractItemModel>
#include <QItemSelectionModel>
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

auto RowRange::get_end_child_number() const -> size_t {
  return first_child_number + number_of_children;
}

auto RowRange::first_is_less(const RowRange &range_2) const -> bool {
  auto range_2_is_chords = range_2.is_chords();
  if (is_chords()) {
    if (range_2_is_chords) {
      return first_child_number < range_2.first_child_number;
    }
    // chord is less than note 2 if they are tied
    return first_child_number <= range_2.get_parent_chord_number();
  }
  auto range_1_chord_number = get_parent_chord_number();
  if (range_2_is_chords) {
    // note isn't less than chord 2 if they are tied
    return range_1_chord_number < range_2.first_child_number;
  }
  return range_1_chord_number < range_2.get_parent_chord_number();
}

auto RowRange::is_chords() const -> bool { return parent_number == -1; }

auto RowRange::get_parent_chord_number() const -> size_t {
  return to_size_t(parent_number);
}

