#include "commands/SetChordTempoRatio.hpp"

#include <QtGlobal>

#include "other/ChordsModel.hpp"

SetChordTempoRatio::SetChordTempoRatio(ChordsModel *chords_model_pointer_input,
                         size_t chord_number_input,
                         const Rational& old_tempo_input,
                       const Rational& new_tempo_input,
                         QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input),
      old_tempo(old_tempo_input),
      new_tempo(new_tempo_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetChordTempoRatio::undo() {
  chords_model_pointer->set_chord_tempo_ratio(chord_number, old_tempo);
}

void SetChordTempoRatio::redo() {
  chords_model_pointer->set_chord_tempo_ratio(chord_number, new_tempo);
}
