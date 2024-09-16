#include "note/SetNoteInstrument.hpp"

#include <QtGlobal>
#include <QList>

#include "justly/NoteColumn.hpp"
#include "note/Note.hpp"
#include "note/NotesModel.hpp"


static void set_note_instrument(NotesModel& notes_model,
                                qsizetype note_number,
                                const Instrument *new_instrument_pointer) {
  auto *notes_pointer = notes_model.notes_pointer;
  Q_ASSERT(notes_pointer != nullptr);
  (*notes_pointer)[note_number].instrument_pointer =
      new_instrument_pointer;
  notes_model.edited_cells(
      note_number, 1, note_instrument_column, note_instrument_column);
}

SetNoteInstrument::SetNoteInstrument(
    NotesModel *notes_model_pointer_input, qsizetype note_number_input,
    const Instrument *old_instrument_pointer_input,
    const Instrument *new_instrument_pointer_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      notes_model_pointer(notes_model_pointer_input),
      note_number(note_number_input),
      old_instrument_pointer(old_instrument_pointer_input),
      new_instrument_pointer(new_instrument_pointer_input) {
  Q_ASSERT(notes_model_pointer != nullptr);
}

void SetNoteInstrument::undo() {
  set_note_instrument(*notes_model_pointer, note_number, old_instrument_pointer);
}

void SetNoteInstrument::redo() {
  set_note_instrument(*notes_model_pointer, note_number, new_instrument_pointer);
}
