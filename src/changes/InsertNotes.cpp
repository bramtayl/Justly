#include "changes/InsertNotes.hpp"

#include <QtGlobal>

#include "justly/ChordsModel.hpp"

InsertNotes::InsertNotes(ChordsModel *chords_model_pointer_input,
                         size_t first_note_number_input,
                         const std::vector<Note> &notes_input,
                         int chord_number_input,
                         QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_note_number(first_note_number_input), notes(notes_input),
      chord_number(chord_number_input) {}

auto InsertNotes::undo() -> void {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->remove_notes(first_note_number, notes.size(),
                                              chord_number);
}

auto InsertNotes::redo() -> void {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->insert_notes(first_note_number, notes,
                                              chord_number);
}
