#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "items_model/ItemsModel.hpp"

template <typename Item> struct InsertItem : public QUndoCommand {
  ItemsModel<Item> *const items_model_pointer;
  const int item_number;
  const Item new_item;

  InsertItem(ItemsModel<Item> *items_model_pointer_input, int item_number_input,
             const Item &new_item_input,
             QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        items_model_pointer(items_model_pointer_input),
        item_number(item_number_input), new_item(new_item_input) {
    Q_ASSERT(items_model_pointer != nullptr);
  };

  void undo() override { items_model_pointer->remove_items(item_number, 1); };
  void redo() override {
    items_model_pointer->insert_item(item_number, new_item);
  };
};
