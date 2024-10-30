#pragma once

#include <QString>

#include "rows/RowsModel.hpp"
#include "unpitched_note/UnpitchedNote.hpp"

class QObject;
class QModelIndex;
class QUndoStack;

struct UnpitchedNotesModel : public RowsModel<UnpitchedNote> {
  explicit UnpitchedNotesModel(QUndoStack& undo_stack,
                               QObject *parent_pointer = nullptr);

  [[nodiscard]] auto
  columnCount(const QModelIndex &parent) const -> int override;

  [[nodiscard]] auto
  get_column_name(int column_number) const -> QString override;
  [[nodiscard]] auto get_status(int row_number) const -> QString override;
};
