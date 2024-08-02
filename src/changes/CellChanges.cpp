#include "changes/CellChanges.hpp"

#include <QtGlobal>

#include "models/ChordsModel.hpp"

CellChanges::CellChanges(
    ChordsModel *chords_model_pointer_input,
    const std::vector<RowRange> &row_replacement_ranges_input,
    NoteChordColumn left_field_input, NoteChordColumn right_field_input,
    const std::vector<NoteChord> &old_note_chords_input,
    const std::vector<NoteChord> &new_note_chords_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      row_ranges(row_replacement_ranges_input),
      left_field(left_field_input), right_field(right_field_input),
      old_note_chords(old_note_chords_input),
      new_note_chords(new_note_chords_input) {}

void CellChanges::undo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->replace_cell_ranges(row_ranges, left_field, right_field, old_note_chords);
}

void CellChanges::redo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->replace_cell_ranges(row_ranges, left_field, right_field, new_note_chords);
}
