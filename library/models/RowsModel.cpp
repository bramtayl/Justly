#include "models/RowsModel.hpp"

auto get_number_of_rows(const QItemSelectionRange &range) -> int {
  Q_ASSERT(range.isValid());
  return range.bottom() - range.top() + 1;
}

auto make_range(QAbstractItemModel &model, const int first_row_number,
                const int number_of_rows, const int left_column,
                const int right_column) -> QItemSelectionRange {
  return QItemSelectionRange(
      model.index(first_row_number, left_column),
      model.index(first_row_number + number_of_rows - 1, right_column));
}
