#pragma once

#include "actions/SetCells.hpp"
#include "models/RowsModel.hpp"

template <RowInterface SubRow>
struct UndoRowsModel : public RowsModel<SubRow> {
  QUndoStack& undo_stack;

  explicit UndoRowsModel(QUndoStack& undo_stack_input, Song& song)
      : RowsModel<SubRow>(song), undo_stack(undo_stack_input) {};

  [[nodiscard]] auto setData(const QModelIndex& index,
                             const QVariant& new_value, const int role)
      -> bool override {
    if (role != Qt::EditRole) {
      return false;
    };
    if (index.data(Qt::EditRole) == new_value) {
      return false;
    }
    if (!this->check_cell(index.column(), new_value)) {
      return false;
    };
    undo_stack.push(make_set_cell(index, new_value));
    return true;
  }

  [[nodiscard]] virtual auto make_set_cell(const QModelIndex& index,
                                           const QVariant& new_value)
      -> QUndoCommand* {
    const auto row_number = index.row();
    const auto column_number = index.column();
    auto new_row = this->get_rows().at(row_number);
    new_row.set_data(column_number, new_value);
    return new SetCells<SubRow>(  // NOLINT(cppcoreguidelines-owning-memory)
        *this, row_number, 1, column_number, column_number, {new_row});
  }
};
