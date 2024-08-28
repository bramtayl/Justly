#include "commands/SetNoteBeats.hpp"

#include <QtGlobal>

#include "other/ChordsModel.hpp"

SetNoteBeats::SetNoteBeats(ChordsModel *chords_model_pointer_input,
                           size_t chord_number_input, size_t note_number_input,
                           const Rational &old_beats_input,
                           const Rational &new_beats_input,
                           QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), note_number(note_number_input),
      old_beats(old_beats_input), new_beats(new_beats_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetNoteBeats::undo() {
  chords_model_pointer->set_note_beats(chord_number, note_number, old_beats);
}

void SetNoteBeats::redo() {
  chords_model_pointer->set_note_beats(chord_number, note_number, new_beats);
}
