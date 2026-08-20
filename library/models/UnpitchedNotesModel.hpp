#pragma once

#include "models/UndoRowsModel.hpp"
#include "rows/UnpitchedNote.hpp"

struct UnpitchedNotesModel : public UndoRowsModel<UnpitchedNote> {
  explicit UnpitchedNotesModel(QUndoStack& undo_stack, Song& song)
      : UndoRowsModel<UnpitchedNote>(undo_stack, song) {}

  [[nodiscard]] auto get_display_data(int row_number, int column_number) const
      -> QVariant override;

  void add_to_status(QTextStream& stream, int /*row_number*/,
                     const UnpitchedNote& unpitched_note) const override;
};
