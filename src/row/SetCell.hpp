#pragma once

#include <QAbstractItemModel>
#include <QUndoStack>
#include <QtGlobal>
#include <utility>

#include "row/Row.hpp"

template <std::derived_from<Row> SubRow> struct RowsModel;

template <std::derived_from<Row> SubRow> struct SetCell : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  QModelIndex index;
  const QVariant old_value;
  const QVariant new_value;

  explicit SetCell(RowsModel<SubRow> &rows_model_input,
                   const QModelIndex &index_input, QVariant new_value_input)
      : rows_model(rows_model_input), index(index_input),
        old_value(rows_model.data(index, Qt::DisplayRole)),
        new_value(std::move(new_value_input)){};

  void undo() override { set_model_data_directly(*this, old_value); };

  void redo() override { set_model_data_directly(*this, new_value); };
};

template <std::derived_from<Row> SubRow>
static void set_model_data_directly(SetCell<SubRow> &change, const QVariant& set_value) {
  const auto &index = change.index;
  change.rows_model.set_cell(index.row(), index.column(), set_value);
}