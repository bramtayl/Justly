#pragma once

#include <QtGlobal>
#include <cstddef>

#include "justly/RowRange.hpp"

RowRange::RowRange(size_t first_child_number_input, size_t number_of_rows_input,
                   int parent_number_input)
    : first_child_number(first_child_number_input),
      number_of_rows(number_of_rows_input), parent_number(parent_number_input) {
}

auto RowRange::operator<(const RowRange &range_2) const -> bool {
  auto range_2_parent_number = range_2.parent_number;
  auto range_2_is_chord = range_2_parent_number == -1;

  if (parent_number == -1) {
    Q_ASSERT(number_of_rows == 1);
    if (range_2_is_chord) {
      // if we have multiple chords, they could surround a note range
      Q_ASSERT(range_2.number_of_rows == 1);
      return first_child_number < range_2.first_child_number;
    }
    // chord is less than note 2 if they are tied
    return first_child_number <= static_cast<size_t>(range_2_parent_number);
  }
  auto range_1_chord_number = static_cast<size_t>(parent_number);
  if (range_2_is_chord) {
    // if we have multiple chords, they could surround a note range
    Q_ASSERT(range_2.number_of_rows == 1);
    // note isn't less than chord 2 if they are tied
    return range_1_chord_number < range_2.first_child_number;
  }
  return range_1_chord_number < static_cast<size_t>(range_2_parent_number);
}