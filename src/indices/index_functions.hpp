#pragma once

#include <cstddef>
#include <vector>

#include "indices/RowRange.hpp"

class QItemSelection;
class QModelIndex;

[[nodiscard]] auto get_child_number(const QModelIndex &index) -> size_t;

[[nodiscard]] auto is_root_index(const QModelIndex &index) -> bool;
[[nodiscard]] auto valid_is_chord_index(const QModelIndex &index) -> bool;

[[nodiscard]] auto get_number_of_rows(const QItemSelection &selection) -> size_t;

[[nodiscard]] auto get_first_row_range(const QItemSelection &selection) -> RowRange;

[[nodiscard]] auto to_row_ranges(const QItemSelection &selection) -> std::vector<RowRange>;

