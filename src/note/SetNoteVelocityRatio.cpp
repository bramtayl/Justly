#include "note/SetNoteVelocityRatio.hpp"

#include <QtGlobal>
#include <QList>

#include "justly/NoteColumn.hpp"
#include "note/Note.hpp"
#include "note/NotesModel.hpp"


static void set_note_velocity_ratio(NotesModel *notes_model_pointer,
                                    qsizetype note_number,
                                    const Rational &new_velocity_ratio) {
  Q_ASSERT(notes_model_pointer != nullptr);
  notes_model_pointer->notes[note_number].velocity_ratio =
      new_velocity_ratio;
  notes_model_pointer->edited_notes_cells(
      note_number, 1, note_velocity_ratio_column, note_velocity_ratio_column);
}

SetNoteVelocityRatio::SetNoteVelocityRatio(
    NotesModel *notes_model_pointer_input, qsizetype note_number_input,
    const Rational &old_velocity_ratio_input,
    const Rational &new_velocity_ratio_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      notes_model_pointer(notes_model_pointer_input),
      note_number(note_number_input),
      old_velocity_ratio(old_velocity_ratio_input),
      new_velocity_ratio(new_velocity_ratio_input) {
  Q_ASSERT(notes_model_pointer != nullptr);
}

void SetNoteVelocityRatio::undo() {
  set_note_velocity_ratio(notes_model_pointer, note_number, old_velocity_ratio);
}

void SetNoteVelocityRatio::redo() {
  set_note_velocity_ratio(notes_model_pointer, note_number, new_velocity_ratio);
}
