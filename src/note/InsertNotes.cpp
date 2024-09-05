#include "note/InsertNotes.hpp"

#include <QtGlobal>

#include "note/NotesModel.hpp"

InsertNotes::InsertNotes(NotesModel *notes_model_pointer_input,
                         qsizetype first_note_number_input,
                         const QList<Note> &new_notes_input,
                         QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      notes_model_pointer(notes_model_pointer_input),
      first_note_number(first_note_number_input), new_notes(new_notes_input) {
  Q_ASSERT(notes_model_pointer != nullptr);
}

auto InsertNotes::undo() -> void {
  remove_notes(notes_model_pointer, first_note_number, new_notes.size());
}

auto InsertNotes::redo() -> void {
  insert_notes(notes_model_pointer, first_note_number, new_notes);
}
