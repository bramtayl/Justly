#include "note/InsertNote.hpp"

#include <QtGlobal>
#include <vector>

#include "note/NotesModel.hpp"
#include "other/templates.hpp"

InsertNote::InsertNote(NotesModel *notes_model_pointer_input,
                       size_t note_number_input, const Note &new_note_input,
                       QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      notes_model_pointer(notes_model_pointer_input),
      note_number(note_number_input), new_note(new_note_input) {
  Q_ASSERT(notes_model_pointer != nullptr);
}

auto InsertNote::undo() -> void {
  remove_notes(notes_model_pointer, note_number, 1);
}

auto InsertNote::redo() -> void {
  Q_ASSERT(notes_model_pointer != nullptr);
  auto &notes = notes_model_pointer->notes;
  check_end_number(notes, note_number);

  notes_model_pointer->begin_insert_rows(note_number, 1);
  notes.insert(notes.begin() + static_cast<int>(note_number), new_note);
  notes_model_pointer->end_insert_rows();
}
