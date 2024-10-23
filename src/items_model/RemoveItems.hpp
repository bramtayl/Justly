#pragma once

#include <QList>
#include <QUndoStack>
#include <QtGlobal>

#include "items_model/ItemsModel.hpp"

template <typename Item> struct RemoveItems : public QUndoCommand {
  ItemsModel<Item> *const items_model_pointer;
  const int first_item_number;
  const QList<Item> old_items;

  RemoveItems(ItemsModel<Item> *items_model_pointer_input,
              int first_item_number_input,
              const QList<Item> &old_items_input,
              QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        items_model_pointer(items_model_pointer_input),
        first_item_number(first_item_number_input), old_items(old_items_input) {
    Q_ASSERT(items_model_pointer != nullptr);
  };

  void undo() override {
    items_model_pointer->insert_items(first_item_number, old_items);
  };
  void redo() override {
    items_model_pointer->remove_items(first_item_number, old_items.size());
  };
};
