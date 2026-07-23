#pragma once

#include <QtCore/QAbstractItemModel>
#include <QtCore/QVariant>
#include <QtCore/Qt>
#include <QtGui/QUndoStack>

#include "actions/SetCell.hpp"
#include "models/RowsModel.hpp"
#include "rows/Row.hpp"

struct Song;

template <RowInterface SubRow> struct UndoRowsModel : public RowsModel<SubRow> {
  QUndoStack &undo_stack;

  explicit UndoRowsModel(QUndoStack &undo_stack_input, Song &song)
      : RowsModel<SubRow>(song), undo_stack(undo_stack_input){};

  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             const int role) -> bool override {
    if (role != Qt::EditRole) {
      return false;
    };
    if (index.data(Qt::EditRole) == new_value) {
      return false;
    }
    if (!this->check_cell(index.column(), new_value)) {
      return false;
    };
    undo_stack.push(
        new SetCell<SubRow>( // NOLINT(cppcoreguidelines-owning-memory)
            *this, index, new_value));
    return true;
  }
};
