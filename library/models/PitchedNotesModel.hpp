#pragma once


#include "models/UndoRowsModel.hpp"
#include "rows/PitchedNote.hpp"

struct PitchedNotesModel : public UndoRowsModel<PitchedNote> {
  explicit PitchedNotesModel(QUndoStack& undo_stack, Song& song)
      : UndoRowsModel<PitchedNote>(undo_stack, song) {}

  [[nodiscard]] auto get_display_data(int row_number, int column_number) const
      -> QVariant override;

  void add_to_status(QTextStream& stream, int /*row_number*/,
                     const PitchedNote& pitched_note) const override;
};
