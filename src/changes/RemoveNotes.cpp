#include "changes/RemoveNotes.hpp"

#include <QtGlobal>

#include "justly/ChordsModel.hpp"

RemoveNotes::RemoveNotes(ChordsModel *chords_model_pointer_input,
                         size_t chord_number_input,
                         size_t first_note_number_input,
                         const std::vector<Note> &notes_input,
                         QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input),
      first_note_number(first_note_number_input), notes(notes_input) {}

auto RemoveNotes::undo() -> void {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->insert_notes(chord_number, first_note_number, notes);
}

auto RemoveNotes::redo() -> void {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->remove_notes(chord_number, first_note_number,
                                     notes.size());
}
