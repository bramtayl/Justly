#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "items_model/ItemsModel.hpp"
#include "rational/Rational.hpp"

template <typename Item>
static void set_beats(ItemsModel<Item> &items_model, int item_number,
                      const Rational &new_beats) {
  auto *items_pointer = items_model.items_pointer;
  Q_ASSERT(items_pointer != nullptr);
  (*items_pointer)[item_number].beats = new_beats;
  auto beats_column = items_model.get_beats_column();
  items_model.edited_cells(item_number, 1, beats_column, beats_column);
}

template <typename Item> struct SetBeats : public QUndoCommand {
  ItemsModel<Item> *const items_model_pointer;
  const int item_number;
  const Rational old_beats;
  const Rational new_beats;

  explicit SetBeats(ItemsModel<Item> *items_model_pointer_input,
                    int item_number_input,
                    const Rational &old_beats_input,
                    const Rational &new_beats_input,
                    QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        items_model_pointer(items_model_pointer_input),
        item_number(item_number_input), old_beats(old_beats_input),
        new_beats(new_beats_input) {};

  void undo() override {
    set_beats(*items_model_pointer, item_number, old_beats);
  };
  
  void redo() override {
    set_beats(*items_model_pointer, item_number, new_beats);
  };
};
