#include "chord/SetChordTempoRatio.hpp"

#include <QList>
#include <QtGlobal>

#include "chord/Chord.hpp"
#include "chord/ChordsModel.hpp"
#include "justly/ChordColumn.hpp"

static void set_chord_tempo_ratio(ChordsModel& chords_model,
                                  qsizetype chord_number,
                                  const Rational &new_tempo_ratio) {
  chords_model.chords[chord_number].tempo_ratio =
      new_tempo_ratio;
  chords_model.edited_cells(
      chord_number, 1, chord_tempo_ratio_column, chord_tempo_ratio_column);
}

SetChordTempoRatio::SetChordTempoRatio(ChordsModel *chords_model_pointer_input,
                                       qsizetype chord_number_input,
                                       const Rational &old_tempo_input,
                                       const Rational &new_tempo_input,
                                       QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), old_tempo(old_tempo_input),
      new_tempo(new_tempo_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetChordTempoRatio::undo() {
  set_chord_tempo_ratio(*chords_model_pointer, chord_number, old_tempo);
}

void SetChordTempoRatio::redo() {
  set_chord_tempo_ratio(*chords_model_pointer, chord_number, new_tempo);
}
