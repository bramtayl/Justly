#pragma once

#include "models/UndoRowsModel.hpp"
#include "rows/Chord.hpp"

struct ChordsModel : public UndoRowsModel<Chord> {
  explicit ChordsModel(QWidget& parent_input, QUndoStack& undo_stack,
                       Song& song_input)
      : UndoRowsModel(parent_input, undo_stack, song_input) {}

  void add_to_status(QTextStream& stream, int row_number,
                     const Chord& chord) const override;
};
