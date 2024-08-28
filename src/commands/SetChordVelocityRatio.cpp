#include "commands/SetChordVelocityRatio.hpp"

#include <QtGlobal>

#include "other/ChordsModel.hpp"

SetChordVelocityRatio::SetChordVelocityRatio(ChordsModel *chords_model_pointer_input,
                         size_t chord_number_input,
                         const Rational& old_velocity_ratio_input, const Rational& new_velocity_ratio_input, 
                         QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input),
      old_velocity_ratio(old_velocity_ratio_input),
      new_velocity_ratio(new_velocity_ratio_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetChordVelocityRatio::undo() {
  chords_model_pointer->set_chord_velocity_ratio(chord_number, old_velocity_ratio);
}

void SetChordVelocityRatio::redo() {
  chords_model_pointer->set_chord_velocity_ratio(chord_number, old_velocity_ratio);
}
