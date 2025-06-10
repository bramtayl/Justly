#pragma once

#include "RowsModel.hpp"

template <RowInterface SubRow> struct SetCell : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  QModelIndex index;
  const QVariant old_value;
  const QVariant new_value;

  explicit SetCell(RowsModel<SubRow> &rows_model_input,
                   const QModelIndex &index_input, QVariant new_value_input)
      : rows_model(rows_model_input), index(index_input),
        old_value(index.data()), new_value(std::move(new_value_input)) {}

  void undo() override { rows_model.set_cell(index, old_value); }

  void redo() override { rows_model.set_cell(index, new_value); }
};
