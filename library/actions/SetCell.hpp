#pragma once

#include <QtCore/QAbstractItemModel>
#include <QtCore/QVariant>
#include <QtCore/Qt>
#include <QtGui/QUndoStack>
#include <utility>

#include "rows/Row.hpp"

template <RowInterface SubRow> struct RowsModel;

template <RowInterface SubRow> struct SetCell : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  QModelIndex index;
  const QVariant old_value;
  const QVariant new_value;

  explicit SetCell(RowsModel<SubRow> &rows_model_input,
                   const QModelIndex &index_input, QVariant new_value_input)
      : rows_model(rows_model_input), index(index_input),
        old_value(index.data(Qt::EditRole)),
        new_value(std::move(new_value_input)) {}

  void undo() override { rows_model.set_cell(index, old_value); }

  void redo() override { rows_model.set_cell(index, new_value); }
};
