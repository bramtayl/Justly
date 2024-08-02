#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QtGlobal>
#include <algorithm>
#include <numeric>

#include "indices/index_functions.hpp"
#include "other/conversions.hpp"

auto get_child_number(const QModelIndex &index) -> size_t {
  return to_size_t(index.row());
}

auto is_root_index(const QModelIndex &index) -> bool {
  // root index is invalid
  return !index.isValid();
}

auto valid_is_chord_index(const QModelIndex &index) -> bool {
  Q_ASSERT(!is_root_index(index));
  // chords have null parent pointers
  return index.internalPointer() == nullptr;
}

auto get_number_of_rows(const QItemSelection &selection) -> size_t {
  return std::accumulate(
      selection.cbegin(), selection.cend(), static_cast<size_t>(0),
      [](size_t total, const QItemSelectionRange &next_range) {
        return total + next_range.bottom() - next_range.top() + 1;
      });
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