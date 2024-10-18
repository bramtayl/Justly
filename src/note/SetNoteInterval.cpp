#include "note/SetNoteInterval.hpp"

#include <QList>
#include <QtGlobal>

#include "justly/NoteColumn.hpp"
#include "note/Note.hpp"
#include "note/NotesModel.hpp"

static void set_note_interval(NotesModel& notes_model,
                              qsizetype note_number,
                              const Interval &new_interval) {
  auto *notes_pointer = notes_model.notes_pointer;
  Q_ASSERT(notes_pointer != nullptr);
  (*notes_pointer)[note_number].interval = new_interval;
  notes_model.edited_cells(note_number, 1, note_interval_column,
                                          note_interval_column);
}

SetNoteInterval::SetNoteInterval(NotesModel *notes_model_pointer_input,
                                 qsizetype note_number_input,
                                 const Interval &old_interval_input,
                                 const Interval &new_interval_input,
                                 QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      notes_model_pointer(notes_model_pointer_input),
      note_number(note_number_input), old_interval(old_interval_input),
      new_interval(new_interval_input) {
  Q_ASSERT(notes_model_pointer != nullptr);
}

void SetNoteInterval::undo() {
  set_note_interval(*notes_model_pointer, note_number, old_interval);
}

void SetNoteInterval::redo() {
  set_note_interval(*notes_model_pointer, note_number, new_interval);
}
