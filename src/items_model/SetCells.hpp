#pragma once

#include <QList>
#include <QUndoStack>
#include <QtGlobal>

#include "items_model/ItemsModel.hpp"

template <typename Item> struct SetCells : public QUndoCommand {
  ItemsModel<Item> *const items_model_pointer;
  const int first_item_number;
  const int left_column;
  const int right_column;
  const QList<Item> old_items;
  const QList<Item> new_items;
  explicit SetCells(ItemsModel<Item> *items_model_pointer_input,
                    int first_item_number_input, int left_column_input,
                    int right_column_input, const QList<Item> &old_items_input,
                    const QList<Item> &new_items_input,
                    QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        items_model_pointer(items_model_pointer_input),
        first_item_number(first_item_number_input),
        left_column(left_column_input), right_column(right_column_input),
        old_items(old_items_input), new_items(new_items_input) {
    Q_ASSERT(items_model_pointer != nullptr);
  };

  void undo() override {
    items_model_pointer->set_cells(first_item_number, left_column, right_column,
                                   old_items);
  };

  void redo() override {
    items_model_pointer->set_cells(first_item_number, left_column, right_column,
                                   new_items);
  }
};
