#pragma once

#include "actions/SetCells.hpp"
#include "cell_types/Interval.hpp"
#include "models/RowsModel.hpp"

template <RowInterface SubRow>
struct UndoRowsModel : public RowsModel<SubRow> {
  // for warnings about rejected edits
  QWidget& parent;
  QUndoStack& undo_stack;

  explicit UndoRowsModel(QWidget& parent_input, QUndoStack& undo_stack_input,
                         Song& song)
      : RowsModel<SubRow>(song),
        parent(parent_input),
        undo_stack(undo_stack_input) {};

  [[nodiscard]] auto setData(const QModelIndex& index,
                             const QVariant& new_value, const int role)
      -> bool override {
    if (role != Qt::EditRole) {
      return false;
    };
    if (index.data(Qt::EditRole) == new_value) {
      return false;
    }
    // the editor allows octaves that the ratio's factors of 2 push out of
    // range once Interval moves them into the octave
    if (new_value.metaType() == QMetaType::fromType<Interval>() &&
        !check_interval(parent, variant_to<Interval>(new_value))) {
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
