#pragma once

#include <QAbstractItemModel>
#include <QUndoStack>
#include <QtGlobal>
#include <utility>

#include "rows/Row.hpp"

template <std::derived_from<Row> SubRow> struct RowsModel;

template <std::derived_from<Row> SubRow> struct SetCell : public QUndoCommand {
  RowsModel<SubRow> *const rows_model_pointer;
  QModelIndex index;
  const QVariant old_value;
  const QVariant new_value;

  explicit SetCell(RowsModel<SubRow>& rows_model,
                   const QModelIndex &index_input, QVariant new_value_input,
                   QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        rows_model_pointer(&rows_model), index(index_input),
        old_value(rows_model.data(index, Qt::DisplayRole)),
        new_value(std::move(new_value_input)){};

  void undo() override {
    rows_model_pointer->set_data_directly(index, old_value);
  };

  void redo() override {
    rows_model_pointer->set_data_directly(index, new_value);
  };
};
