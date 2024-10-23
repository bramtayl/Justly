#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "interval/Interval.hpp"
#include "items_model/ItemsModel.hpp"

struct NotesModel;

template <typename Item>
static void set_interval(ItemsModel<Item> &items_model, int item_number,
                         const Interval &new_interval) {
  auto *items_pointer = items_model.items_pointer;
  Q_ASSERT(items_pointer != nullptr);
  (*items_pointer)[item_number].interval = new_interval;
  auto interval_column = items_model.get_interval_column();
  items_model.edited_cells(item_number, 1, interval_column, interval_column);
}

template <typename Item> struct SetInterval : public QUndoCommand {
  ItemsModel<Item> *const items_model_pointer;
  const int item_number;
  const Interval old_interval;
  const Interval new_interval;

  explicit SetInterval(ItemsModel<Item> *items_model_pointer_input,
                       int item_number_input,
                       const Interval &old_interval_input,
                       const Interval &new_interval_input,
                       QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        items_model_pointer(items_model_pointer_input),
        item_number(item_number_input), old_interval(old_interval_input),
        new_interval(new_interval_input) {
    Q_ASSERT(items_model_pointer != nullptr);
  };

  void undo() override {
    set_interval(*items_model_pointer, item_number, old_interval);
  };
  void redo() override {
    set_interval(*items_model_pointer, item_number, new_interval);
  };
};
