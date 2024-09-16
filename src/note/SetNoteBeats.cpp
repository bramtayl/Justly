#include "note/SetNoteBeats.hpp"

#include <QtGlobal>
#include <QList>

#include "justly/NoteColumn.hpp"
#include "note/Note.hpp"
#include "note/NotesModel.hpp"

static void set_note_beats(NotesModel& notes_model, qsizetype note_number,
                           const Rational &new_beats) {
  auto *notes_pointer = notes_model.notes_pointer;
  Q_ASSERT(notes_pointer != nullptr);
  (*notes_pointer)[note_number].beats = new_beats;
  notes_model.edited_cells(note_number, 1, note_beats_column,
                                          note_beats_column);
}

SetNoteBeats::SetNoteBeats(NotesModel *notes_model_pointer_input,
                           qsizetype note_number_input,
                           const Rational &old_beats_input,
                           const Rational &new_beats_input,
                           QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      notes_model_pointer(notes_model_pointer_input),
      note_number(note_number_input), old_beats(old_beats_input),
      new_beats(new_beats_input) {
  Q_ASSERT(notes_model_pointer != nullptr);
}

void SetNoteBeats::undo() {
  set_note_beats(*notes_model_pointer, note_number, old_beats);
}

void SetNoteBeats::redo() {
  set_note_beats(*notes_model_pointer, note_number, new_beats);
}
