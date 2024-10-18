#include "note/InsertNote.hpp"

#include <QList>
#include <QtGlobal>

#include "note/NotesModel.hpp"

InsertNote::InsertNote(NotesModel *notes_model_pointer_input,
                       qsizetype note_number_input, const Note &new_note_input,
                       QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      notes_model_pointer(notes_model_pointer_input),
      note_number(note_number_input), new_note(new_note_input) {
  Q_ASSERT(notes_model_pointer != nullptr);
}

auto InsertNote::undo() -> void {
  remove_notes(*notes_model_pointer, note_number, 1);
}

auto InsertNote::redo() -> void {
  auto *notes_pointer = notes_model_pointer->notes_pointer;
  Q_ASSERT(notes_pointer != nullptr);

  notes_model_pointer->begin_insert_rows(note_number, 1);
  notes_pointer->insert(notes_pointer->begin() + static_cast<int>(note_number), new_note);
  notes_model_pointer->end_insert_rows();
}
