#pragma once

#include <QAbstractItemModel>
#include <QUndoStack>
#include <QtGlobal>
#include <utility>

#include "rows/Row.hpp"

template <std::derived_from<Row> SubRow> struct RowsModel;

template <std::derived_from<Row> SubRow> struct SetCell : public QUndoCommand {
  RowsModel<SubRow>& rows_model;
  QModelIndex index;
  const QVariant old_value;
  const QVariant new_value;

  explicit SetCell(RowsModel<SubRow>& rows_model_input,
                   const QModelIndex &index_input, QVariant new_value_input)
      : rows_model(rows_model_input), index(index_input),
        old_value(rows_model.data(index, Qt::DisplayRole)),
        new_value(std::move(new_value_input)){};

  void undo() override {
    set_model_data_directly(rows_model, index, old_value);
  };

  void redo() override {
    set_model_data_directly(rows_model, index, new_value);
  };
};
