#pragma once

#include <QString>
#include <QUndoStack>
#include <QtGlobal>

#include "items_model/ItemsModel.hpp"

template <typename Item>
static void set_words(ItemsModel<Item> &items_model, int item_number,
                      const QString &new_words) {
  auto *items_pointer = items_model.items_pointer;
  Q_ASSERT(items_pointer != nullptr);
  (*items_pointer)[item_number].words = new_words;
  auto words_column = items_model.get_words_column();
  items_model.edited_cells(item_number, 1, words_column, words_column);
}

template <typename Item> struct SetWords : public QUndoCommand {
  ItemsModel<Item> *const items_model_pointer;
  const int item_number;
  const QString old_words;
  const QString new_words;

  explicit SetWords(ItemsModel<Item> *items_model_pointer_input,
                    int item_number_input, QString old_words_input,
                    QString new_words_input,
                    QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        items_model_pointer(items_model_pointer_input),
        item_number(item_number_input), old_words(std::move(old_words_input)),
        new_words(std::move(new_words_input)) {
    Q_ASSERT(items_model_pointer != nullptr);
  };

  void undo() override {
    set_words(*items_model_pointer, item_number, old_words);
  };
  void redo() override {
    set_words(*items_model_pointer, item_number, new_words);
  };
};
