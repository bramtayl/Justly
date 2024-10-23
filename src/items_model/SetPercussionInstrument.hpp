#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "items_model/ItemsModel.hpp"

struct PercussionInstrument;

template <typename Item>
static void set_percussion_instrument(
    ItemsModel<Item> &items_model, int item_number,
    const PercussionInstrument *new_percussion_instrument_pointer) {
  auto *items_pointer = items_model.items_pointer;
  Q_ASSERT(items_pointer != nullptr);
  (*items_pointer)[item_number].percussion_instrument_pointer =
      new_percussion_instrument_pointer;
  auto percussion_instrument_column =
      items_model.get_percussion_instrument_column();
  items_model.edited_cells(item_number, 1, percussion_instrument_column,
                           percussion_instrument_column);
}

template <typename Item> struct SetPercussionInstrument : public QUndoCommand {
  ItemsModel<Item> *const items_model_pointer;
  const int item_number;
  const PercussionInstrument *const old_percussion_instrument_pointer;
  const PercussionInstrument *const new_percussion_instrument_pointer;

  explicit SetPercussionInstrument(
      ItemsModel<Item> *items_model_pointer_input, int item_number_input,
      const PercussionInstrument *old_percussion_instrument_pointer_input,
      const PercussionInstrument *new_percussion_instrument_pointer_input,
      QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        items_model_pointer(items_model_pointer_input),
        item_number(item_number_input),
        old_percussion_instrument_pointer(
            old_percussion_instrument_pointer_input),
        new_percussion_instrument_pointer(
            new_percussion_instrument_pointer_input) {
    Q_ASSERT(items_model_pointer != nullptr);
  };

  void undo() override {
    set_percussion_instrument(*items_model_pointer, item_number,
                              old_percussion_instrument_pointer);
  };
  void redo() override {
    set_percussion_instrument(*items_model_pointer, item_number,
                              new_percussion_instrument_pointer);
  };
};
