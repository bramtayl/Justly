#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QtGlobal>
#include <algorithm>
#include <cstddef>
#include <vector>

#include "justly/RowRange.hpp"
#include "other/private.hpp"

RowRange::RowRange(size_t first_child_number_input, size_t number_of_rows_input,
                   int parent_number_input)
    : first_child_number(first_child_number_input),
      number_of_children(number_of_rows_input),
      parent_number(parent_number_input) {}

RowRange::RowRange(const QItemSelectionRange &range)
    : first_child_number(to_size_t(range.top())),
      number_of_children(to_size_t(range.bottom() - range.top() + 1)),
      parent_number(range.parent().row()) {}

auto RowRange::is_chords() const -> bool { return parent_number == -1; }

auto RowRange::operator<(const RowRange &range_2) const -> bool {
  auto range_2_parent_number = range_2.parent_number;
  auto range_2_is_chords = range_2.is_chords();
  if (is_chords()) {
    Q_ASSERT(number_of_children == 1);
    if (range_2_is_chords) {
      // if we have multiple chords, they could surround a note range
      Q_ASSERT(range_2.number_of_children == 1);
      return first_child_number < range_2.first_child_number;
    }
    // chord is less than note 2 if they are tied
    return first_child_number <= static_cast<size_t>(range_2_parent_number);
  }
  auto range_1_chord_number = static_cast<size_t>(parent_number);
  if (range_2_is_chords) {
    // if we have multiple chords, they could surround a note range
    Q_ASSERT(range_2.number_of_children == 1);
    // note isn't less than chord 2 if they are tied
    return range_1_chord_number < range_2.first_child_number;
  }
  return range_1_chord_number < static_cast<size_t>(range_2_parent_number);
}

auto to_row_ranges(const QItemSelection &selection) -> std::vector<RowRange> {
  std::vector<RowRange> row_ranges;
  for (const auto &range : selection) {
    const auto row_range = RowRange(range);
    auto parent_number = row_range.parent_number;
    if (parent_number == -1) {
      auto first_child_number = row_range.first_child_number;
      // separate chords because there are notes in between
      auto end_child_number = first_child_number + row_range.number_of_children;
      for (auto child_number = first_child_number;
           child_number < end_child_number; child_number++) {
        row_ranges.emplace_back(child_number, 1, parent_number);
      }
    } else {
      row_ranges.push_back(row_range);
    }
  }
  // sort to collate chords and notes
  std::sort(row_ranges.begin(), row_ranges.end());
  return row_ranges;
}