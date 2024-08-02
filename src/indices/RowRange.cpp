#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QtGlobal>
#include <algorithm>
#include <cstddef>
#include <numeric>
#include <vector>

#include "justly/RowRange.hpp"
#include "other/conversions.hpp"

auto get_number_of_rows(const QItemSelection &selection) -> size_t {
  return std::accumulate(
      selection.cbegin(), selection.cend(), static_cast<size_t>(0),
      [](size_t total, const QItemSelectionRange &next_range) {
        return total + next_range.bottom() - next_range.top() + 1;
      });
}

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

auto get_first_row_range(const QItemSelection &selection) -> RowRange {
  auto min_pointer = std::min_element(selection.begin(), selection.end(),
                                      [](const QItemSelectionRange &range_1,
                                         const QItemSelectionRange &range_2) {
                                        return RowRange(range_1).first_is_less(
                                            RowRange(range_2));
                                      });
  Q_ASSERT(min_pointer != nullptr);
  return RowRange(*min_pointer);
}

auto to_row_ranges(const QItemSelection &selection) -> std::vector<RowRange> {
  std::vector<RowRange> row_ranges;
  for (const auto &range : selection) {
    const auto row_range = RowRange(range);
    auto parent_number = row_range.parent_number;
    if (parent_number == -1) {
      // separate chords because there are notes in between
      auto end_child_number = row_range.get_end_child_number();
      for (auto child_number = row_range.first_child_number;
           child_number < end_child_number; child_number++) {
        row_ranges.emplace_back(child_number, 1, parent_number);
      }
    } else {
      row_ranges.push_back(row_range);
    }
  }
  // sort to collate chords and notes
  std::sort(row_ranges.begin(), row_ranges.end(),
            [](const RowRange &row_range_1, const RowRange &row_range_2) {
              return row_range_1.first_is_less(row_range_2);
            });
  return row_ranges;
}
