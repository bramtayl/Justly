#include "commands/SetNoteVelocityRatio.hpp"

#include <QtGlobal>

#include "justly/NoteChordColumn.hpp"
#include "note_chord/Chord.hpp"
#include "note_chord/Note.hpp"
#include "other/ChordsModel.hpp"
#include "other/templates.hpp"

static void set_note_velocity_ratio(ChordsModel *chords_model_pointer,
                                    size_t chord_number, size_t note_number,
                                    const Rational &new_velocity_ratio) {
  Q_ASSERT(chords_model_pointer != nullptr);
  get_item(get_item(chords_model_pointer->chords, chord_number).notes,
           note_number)
      .velocity_ratio = new_velocity_ratio;
  chords_model_pointer->edited_notes_cells(chord_number, note_number, 1,
                                           velocity_ratio_column,
                                           velocity_ratio_column);
}

SetNoteVelocityRatio::SetNoteVelocityRatio(
    ChordsModel *chords_model_pointer_input, size_t chord_number_input,
    size_t note_number_input, const Rational &old_velocity_ratio_input,
    const Rational &new_velocity_ratio_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), note_number(note_number_input),
      old_velocity_ratio(old_velocity_ratio_input),
      new_velocity_ratio(new_velocity_ratio_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetNoteVelocityRatio::undo() {
  set_note_velocity_ratio(chords_model_pointer, chord_number, note_number,
                          old_velocity_ratio);
}

void SetNoteVelocityRatio::redo() {
  set_note_velocity_ratio(chords_model_pointer, chord_number, note_number,
                          new_velocity_ratio);
}
