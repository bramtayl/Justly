#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "items_model/ItemsModel.hpp"
#include "rational/Rational.hpp"

template <typename Item>
static void set_tempo_ratio(ItemsModel<Item> &items_model, int item_number,
                            const Rational &new_tempo_ratio) {
  auto *items_pointer = items_model.items_pointer;
  Q_ASSERT(items_pointer != nullptr);
  (*items_pointer)[item_number].tempo_ratio = new_tempo_ratio;
  auto tempo_ratio_column = items_model.get_tempo_ratio_column();
  items_model.edited_cells(item_number, 1, tempo_ratio_column,
                           tempo_ratio_column);
}

template <typename Item> struct SetTempoRatio : public QUndoCommand {
  ItemsModel<Item> *const items_model_pointer;
  const int item_number;
  const Rational old_tempo;
  const Rational new_tempo;

  explicit SetTempoRatio(ItemsModel<Item> *items_model_pointer_input,
                         int item_number_input,
                         const Rational &old_tempo_input,
                         const Rational &new_tempo_input,
                         QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        items_model_pointer(items_model_pointer_input),
        item_number(item_number_input), old_tempo(old_tempo_input),
        new_tempo(new_tempo_input) {
    Q_ASSERT(items_model_pointer != nullptr);
  };

  void undo() override {
    set_tempo_ratio(*items_model_pointer, item_number, old_tempo);
  };
  void redo() override {
    set_tempo_ratio(*items_model_pointer, item_number, new_tempo);
  };
};
