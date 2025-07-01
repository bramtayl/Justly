#pragma once

#include "actions/SetCell.hpp"

template <RowInterface SubRow> struct UndoRowsModel : public RowsModel<SubRow> {
  QUndoStack &undo_stack;

  explicit UndoRowsModel(QUndoStack &undo_stack_input)
      : undo_stack(undo_stack_input){};

  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             const int role) -> bool override {
    if (role != Qt::EditRole) {
      return false;
    };
    undo_stack.push(
        new SetCell<SubRow>( // NOLINT(cppcoreguidelines-owning-memory)
            *this, index, new_value));
    return true;
  }
};
