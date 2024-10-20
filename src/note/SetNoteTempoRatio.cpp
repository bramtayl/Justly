#include "note/SetNoteTempoRatio.hpp"

#include <QList>
#include <QtGlobal>

#include "justly/NoteColumn.hpp"
#include "note/Note.hpp"
#include "note/NotesModel.hpp"

static void set_note_tempo_ratio(NotesModel& notes_model,
                                 qsizetype note_number,
                                 const Rational &new_tempo_ratio) {
  auto *notes_pointer = notes_model.notes_pointer;
  Q_ASSERT(notes_pointer != nullptr);
  (*notes_pointer)[note_number].tempo_ratio =
      new_tempo_ratio;
  notes_model.edited_cells(
      note_number, 1, note_tempo_ratio_column, note_tempo_ratio_column);
}

SetNoteTempoRatio::SetNoteTempoRatio(NotesModel *notes_model_pointer_input,
                                     qsizetype note_number_input,
                                     const Rational &old_tempo_input,
                                     const Rational &new_tempo_input,
                                     QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      notes_model_pointer(notes_model_pointer_input),
      note_number(note_number_input), old_tempo(old_tempo_input),
      new_tempo(new_tempo_input) {
  Q_ASSERT(notes_model_pointer != nullptr);
}

void SetNoteTempoRatio::undo() {
  set_note_tempo_ratio(*notes_model_pointer, note_number, old_tempo);
}

void SetNoteTempoRatio::redo() {
  set_note_tempo_ratio(*notes_model_pointer, note_number, new_tempo);
}
