#include "note/RemoveNotes.hpp"

#include <QtGlobal>

#include "note/NotesModel.hpp"

RemoveNotes::RemoveNotes(NotesModel *notes_model_pointer_input,
                         qsizetype first_note_number_input,
                         const QList<Note> &old_notes_input,
                         QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      notes_model_pointer(notes_model_pointer_input),
      first_note_number(first_note_number_input), old_notes(old_notes_input) {
  Q_ASSERT(notes_model_pointer != nullptr);
}

auto RemoveNotes::undo() -> void {
  insert_notes(notes_model_pointer, first_note_number, old_notes);
}

auto RemoveNotes::redo() -> void {
  remove_notes(notes_model_pointer, first_note_number, old_notes.size());
}
