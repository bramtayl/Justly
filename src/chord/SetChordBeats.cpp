#include "chord/SetChordBeats.hpp"

#include <QtGlobal>
#include <QList>

#include "chord/Chord.hpp"
#include "chord/ChordsModel.hpp"
#include "justly/ChordColumn.hpp"


static void set_chord_beats(ChordsModel& chords_model,
                            qsizetype chord_number, const Rational &new_beats) {
  chords_model.chords[chord_number].beats = new_beats;
  chords_model.edited_chords_cells(chord_number, 1, chord_beats_column,
                                            chord_beats_column);
}

SetChordBeats::SetChordBeats(ChordsModel *chords_model_pointer_input,
                             qsizetype chord_number_input,
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
  set_chord_beats(*chords_model_pointer, chord_number, old_beats);
}

void SetChordBeats::redo() {
  set_chord_beats(*chords_model_pointer, chord_number, new_beats);
}
