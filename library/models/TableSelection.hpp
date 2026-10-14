#pragma once

#include "rows/RowType.hpp"

// a contiguous range of rows in the switch table; number_of_rows == 0 means
// nothing is selected
struct TableSelection {
  RowType row_type = RowType::chord_type;
  int chord_number = -1;  // -1 unless row_type is a note type
  int first_row_number = -1;
  int number_of_rows = 0;
};
