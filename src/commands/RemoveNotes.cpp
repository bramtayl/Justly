#include "commands/RemoveNotes.hpp"

#include <QtGlobal>

#include "other/ChordsModel.hpp"

RemoveNotes::RemoveNotes(ChordsModel *chords_model_pointer_input,
                         size_t chord_number_input,
                         size_t first_note_number_input,
                         const std::vector<Note> &old_notes_input,
                         QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input),
      first_note_number(first_note_number_input), old_notes(old_notes_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

auto RemoveNotes::undo() -> void {
  insert_notes(chords_model_pointer, chord_number, first_note_number,
               old_notes);
}

auto RemoveNotes::redo() -> void {
  remove_notes(chords_model_pointer, chord_number, first_note_number,
               old_notes.size());
}
