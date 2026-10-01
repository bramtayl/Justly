#pragma once

#include "models/UndoRowsModel.hpp"
#include "rows/PitchedNote.hpp"

struct PitchedNotesModel : public UndoRowsModel<PitchedNote> {
  explicit PitchedNotesModel(QUndoStack& undo_stack, Song& song)
      : UndoRowsModel<PitchedNote>(undo_stack, song) {}

  [[nodiscard]] auto make_empty_row() const -> PitchedNote override;

  void add_to_status(QTextStream& stream, int /*row_number*/,
                     const PitchedNote& pitched_note) const override;
};
