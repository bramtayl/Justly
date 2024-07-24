#include "changes/ChordCellChanges.hpp"

#include <QtGlobal>

#include "justly/ChordsModel.hpp"
#include "justly/NoteChord.hpp"

ChordCellChanges::ChordCellChanges(
    ChordsModel *chords_model_pointer_input, size_t chord_number_input,
    NoteChordField left_field_input, NoteChordField right_field_input,
    const std::vector<NoteChord> &old_note_chords_input,
    const std::vector<NoteChord> &new_note_chords_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), left_field(left_field_input),
      right_field(right_field_input), old_note_chords(old_note_chords_input),
      new_note_chords(new_note_chords_input) {}

void ChordCellChanges::undo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->replace_chords_cells(chord_number, left_field,
                                             right_field, old_note_chords);
}

void ChordCellChanges::redo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->replace_chords_cells(chord_number, left_field,
                                             right_field, new_note_chords);
}
