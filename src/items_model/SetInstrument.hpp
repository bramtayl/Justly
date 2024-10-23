#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "items_model/ItemsModel.hpp"

struct Instrument;

template <typename Item>
static void set_instrument(ItemsModel<Item> &items_model, int item_number,
                           const Instrument *new_instrument_pointer) {
  auto *items_pointer = items_model.items_pointer;
  Q_ASSERT(items_pointer != nullptr);
  (*items_pointer)[item_number].instrument_pointer = new_instrument_pointer;
  auto instrument_column = items_model.get_instrument_column();
  items_model.edited_cells(item_number, 1, instrument_column, instrument_column);
}

template <typename Item> struct SetInstrument : public QUndoCommand {
  ItemsModel<Item> *const items_model_pointer;
  const int item_number;
  const Instrument *const old_instrument_pointer;
  const Instrument *const new_instrument_pointer;

  explicit SetInstrument(ItemsModel<Item> *items_model_pointer_input,
                         int item_number_input,
                         const Instrument *old_instrument_pointer_input,
                         const Instrument *new_instrument_pointer_input,
                         QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        items_model_pointer(items_model_pointer_input),
        item_number(item_number_input),
        old_instrument_pointer(old_instrument_pointer_input),
        new_instrument_pointer(new_instrument_pointer_input) {
    Q_ASSERT(items_model_pointer != nullptr);
  };

  void undo() override {
    set_instrument(*items_model_pointer, item_number,
                        old_instrument_pointer);
  };
  void redo() override {
    set_instrument(*items_model_pointer, item_number,
                        new_instrument_pointer);
  };
};
