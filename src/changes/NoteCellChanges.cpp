#include "changes/NoteCellChanges.hpp"

#include <QtGlobal>

#include "justly/ChordsModel.hpp"
#include "justly/NoteChord.hpp"

NoteCellChanges::NoteCellChanges(ChordsModel *chords_model_pointer_input,
                               size_t chord_number_input,
                               size_t first_note_number_input,
                               NoteChordField left_note_chord_field_input,
                               NoteChordField right_note_chord_field_input,
                               const std::vector<NoteChord>& old_note_chord_templates_input,
                               const std::vector<NoteChord>& new_note_chord_templates_input,
                               QUndoCommand *parent_pointer_input
                               )
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), first_note_number(first_note_number_input),
      left_note_chord_field(left_note_chord_field_input),
      right_note_chord_field(right_note_chord_field_input),
      old_note_chord_templates(old_note_chord_templates_input),
      new_note_chord_templates(new_note_chord_templates_input) {}

void NoteCellChanges::undo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->replace_notes_cells(chord_number, first_note_number, left_note_chord_field, right_note_chord_field, old_note_chord_templates);
}

void NoteCellChanges::redo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->replace_notes_cells(chord_number, first_note_number, left_note_chord_field, right_note_chord_field, new_note_chord_templates);
}
