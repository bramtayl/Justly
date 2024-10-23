#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "items_model/ItemsModel.hpp"
#include "rational/Rational.hpp"

template <typename Item>
static void set_velocity_ratio(ItemsModel<Item> &items_model,
                               int item_number,
                               const Rational &new_velocity_ratio) {
  auto *items_pointer = items_model.items_pointer;
  Q_ASSERT(items_pointer != nullptr);
  (*items_pointer)[item_number].velocity_ratio = new_velocity_ratio;
  auto velocity_ratio_column = items_model.get_velocity_ratio_column();
  items_model.edited_cells(item_number, 1, velocity_ratio_column,
                           velocity_ratio_column);
}

template <typename Item> struct SetVelocityRatio : public QUndoCommand {
  ItemsModel<Item> *const items_model_pointer;
  const int item_number;
  const Rational old_velocity_ratio;
  const Rational new_velocity_ratio;

  explicit SetVelocityRatio(ItemsModel<Item> *items_model_pointer_input,
                            int item_number_input,
                            const Rational &old_velocity_ratio_input,
                            const Rational &new_velocity_ratio_input,
                            QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        items_model_pointer(items_model_pointer_input),
        item_number(item_number_input),
        old_velocity_ratio(old_velocity_ratio_input),
        new_velocity_ratio(new_velocity_ratio_input) {
    Q_ASSERT(items_model_pointer != nullptr);
  };

  void undo() override {
    set_velocity_ratio(*items_model_pointer, item_number, old_velocity_ratio);
  };
  void redo() override {
    set_velocity_ratio(*items_model_pointer, item_number, new_velocity_ratio);
  };
};
