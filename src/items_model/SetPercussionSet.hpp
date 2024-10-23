#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "items_model/ItemsModel.hpp"

struct PercussionSet;

template <typename Item>
static void
set_percussion_set(ItemsModel<Item> &items_model, int item_number,
                   const PercussionSet *new_percussion_set_pointer) {
  auto *items_pointer = items_model.items_pointer;
  Q_ASSERT(items_pointer != nullptr);
  (*items_pointer)[item_number].percussion_set_pointer =
      new_percussion_set_pointer;
  auto percussion_set_column = items_model.get_percussion_set_column();
  items_model.edited_cells(item_number, 1, percussion_set_column,
                           percussion_set_column);
}

template <typename Item> struct SetPercussionSet : public QUndoCommand {
  ItemsModel<Item> *const items_model_pointer;
  const int item_number;
  const PercussionSet *const old_percussion_set_pointer;
  const PercussionSet *const new_percussion_set_pointer;

  explicit SetPercussionSet(
      ItemsModel<Item> *items_model_pointer_input, int item_number_input,
      const PercussionSet *old_percussion_set_pointer_input,
      const PercussionSet *new_percussion_set_pointer_input,
      QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        items_model_pointer(items_model_pointer_input),
        item_number(item_number_input),
        old_percussion_set_pointer(old_percussion_set_pointer_input),
        new_percussion_set_pointer(new_percussion_set_pointer_input) {
    Q_ASSERT(items_model_pointer != nullptr);
  };

  void undo() override {
    set_percussion_set(*items_model_pointer, item_number,
                       old_percussion_set_pointer);
  };
  void redo() override {
    set_percussion_set(*items_model_pointer, item_number,
                       new_percussion_set_pointer);
  };
};
