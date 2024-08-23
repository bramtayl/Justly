#include "commands/InsertNotes.hpp"

#include <QtGlobal>

#include "other/ChordsModel.hpp"

InsertNotes::InsertNotes(ChordsModel *chords_model_pointer_input,
                         size_t chord_number_input,
                         size_t first_note_number_input,
                         const std::vector<Note> &new_notes_input,
                         QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input),
      first_note_number(first_note_number_input), new_notes(new_notes_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

auto InsertNotes::undo() -> void {
  chords_model_pointer->remove_notes(chord_number, first_note_number,
                                     new_notes.size());
}

auto InsertNotes::redo() -> void {
  chords_model_pointer->insert_notes(chord_number, first_note_number, new_notes);
}
