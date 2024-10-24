#pragma once

#include <QUndoStack>
#include <QtGlobal>
#include <qabstractitemmodel.h>
#include <utility>

template <typename T> struct ItemsModel;

template <typename Item> struct SetCell : public QUndoCommand {
  ItemsModel<Item> *const items_model_pointer;
  QModelIndex index;
  const QVariant old_value;
  const QVariant new_value;

  explicit SetCell(ItemsModel<Item> *items_model_pointer_input,
                   const QModelIndex &index_input, QVariant new_value_input,
                   QUndoCommand *parent_pointer_input = nullptr)
      : QUndoCommand(parent_pointer_input),
        items_model_pointer(items_model_pointer_input), index(index_input),
        old_value(items_model_pointer->data(index, Qt::DisplayRole)),
        new_value(std::move(new_value_input)){};

  void undo() override {
    items_model_pointer->set_data_directly(index, old_value);
  };

  void redo() override {
    items_model_pointer->set_data_directly(index, new_value);
  };
};
