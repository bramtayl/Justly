#include "commands/SetCells.hpp"

#include <QtGlobal>

#include "models/ChordsModel.hpp"

SetCells::SetCells(ChordsModel *chords_model_pointer_input,
                   const std::vector<RowRange> &row_ranges_input,
                   const std::vector<NoteChord> &old_note_chords_input,
                   const std::vector<NoteChord> &new_note_chords_input,
                   NoteChordColumn left_column_input,
                   NoteChordColumn right_column_input,
                   QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      row_ranges(row_ranges_input), old_note_chords(old_note_chords_input),
      new_note_chords(new_note_chords_input), left_column(left_column_input),
      right_column(right_column_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetCells::undo() {
  chords_model_pointer->replace_cell_ranges(row_ranges, old_note_chords,
                                            left_column, right_column);
}

void SetCells::redo() {
  chords_model_pointer->replace_cell_ranges(row_ranges, new_note_chords,
                                            left_column, right_column);
}
