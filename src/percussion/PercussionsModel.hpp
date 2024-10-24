#pragma once

#include <QString>

#include "items_model/ItemsModel.hpp"
#include "percussion/Percussion.hpp"

class QObject;
class QModelIndex;
class QUndoStack;
template <typename T> class QList;

struct PercussionsModel : public ItemsModel<Percussion> {
  QList<Percussion> *items_pointer = nullptr;

  explicit PercussionsModel(QUndoStack *undo_stack_pointer_input,
                            QObject *parent_pointer = nullptr);

  [[nodiscard]] auto
  columnCount(const QModelIndex &parent) const -> int override;

  [[nodiscard]] auto
  get_column_name(int column_number) const -> QString override;
  [[nodiscard]] auto get_status(int row_number) const -> QString override;
};
