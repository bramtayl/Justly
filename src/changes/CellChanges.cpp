#include "changes/CellChanges.hpp"

#include <QtGlobal>

#include "justly/ChordsModel.hpp"
#include "justly/NoteChord.hpp"

CellChanges::CellChanges(
    ChordsModel *chords_model_pointer_input, const CellIndex &top_left_cell_index_input,
                       NoteChordField right_field_input,
    const std::vector<NoteChord> &old_note_chords_input,
    const std::vector<NoteChord> &new_note_chords_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      top_left_cell_index(top_left_cell_index_input), right_field(right_field_input),
      old_note_chords(old_note_chords_input),
      new_note_chords(new_note_chords_input) {}

void CellChanges::undo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->replace_cells(top_left_cell_index, right_field, old_note_chords);
}

void CellChanges::redo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->replace_cells(top_left_cell_index, right_field, new_note_chords);
}
