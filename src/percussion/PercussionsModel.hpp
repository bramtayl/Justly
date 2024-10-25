#pragma once

#include <QString>

#include "rows/RowsModel.hpp"
#include "percussion/Percussion.hpp"

class QObject;
class QModelIndex;
class QUndoStack;

struct PercussionsModel : public RowsModel<Percussion> {
  explicit PercussionsModel(QUndoStack *undo_stack_pointer_input,
                            QObject *parent_pointer = nullptr);

  [[nodiscard]] auto
  columnCount(const QModelIndex &parent) const -> int override;

  [[nodiscard]] auto
  get_column_name(int column_number) const -> QString override;
  [[nodiscard]] auto get_status(int row_number) const -> QString override;
};
