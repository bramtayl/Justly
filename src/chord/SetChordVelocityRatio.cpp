#include "chord/SetChordVelocityRatio.hpp"

#include <QtGlobal>
#include <QList>

#include "chord/Chord.hpp"
#include "chord/ChordsModel.hpp"
#include "justly/ChordColumn.hpp"

static void set_chord_velocity_ratio(ChordsModel& chords_model,
                                     qsizetype chord_number,
                                     const Rational &new_velocity_ratio) {
  chords_model.chords[chord_number].velocity_ratio =
      new_velocity_ratio;
  chords_model.edited_cells(chord_number, 1,
                                            chord_velocity_ratio_column,
                                            chord_velocity_ratio_column);
}

SetChordVelocityRatio::SetChordVelocityRatio(
    ChordsModel *chords_model_pointer_input, qsizetype chord_number_input,
    const Rational &old_velocity_ratio_input,
    const Rational &new_velocity_ratio_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input),
      old_velocity_ratio(old_velocity_ratio_input),
      new_velocity_ratio(new_velocity_ratio_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetChordVelocityRatio::undo() {
  set_chord_velocity_ratio(*chords_model_pointer, chord_number,
                           old_velocity_ratio);
}

void SetChordVelocityRatio::redo() {
  set_chord_velocity_ratio(*chords_model_pointer, chord_number,
                           new_velocity_ratio);
}
