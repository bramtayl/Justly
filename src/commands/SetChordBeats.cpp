#include "commands/SetChordBeats.hpp"

#include <QtGlobal>

#include "justly/NoteChordColumn.hpp"
#include "note_chord/Chord.hpp"
#include "other/ChordsModel.hpp"
#include "other/templates.hpp"

static void set_chord_beats(ChordsModel *chords_model_pointer,
                            size_t chord_number, const Rational &new_beats) {
  Q_ASSERT(chords_model_pointer != nullptr);
  get_item(chords_model_pointer->chords, chord_number).beats = new_beats;
  chords_model_pointer->edited_chords_cells(chord_number, 1, beats_column,
                                            beats_column);
}

SetChordBeats::SetChordBeats(ChordsModel *chords_model_pointer_input,
                             size_t chord_number_input,
                             const Rational &old_beats_input,
                             const Rational &new_beats_input,
                             QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), old_beats(old_beats_input),
      new_beats(new_beats_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetChordBeats::undo() {
  set_chord_beats(chords_model_pointer, chord_number, old_beats);
}

void SetChordBeats::redo() {
  set_chord_beats(chords_model_pointer, chord_number, new_beats);
}
