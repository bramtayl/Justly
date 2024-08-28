#include "commands/SetChordInterval.hpp"

#include <QtGlobal>

#include "other/ChordsModel.hpp"

SetChordInterval::SetChordInterval(ChordsModel *chords_model_pointer_input,
                                 size_t chord_number_input,
                                 const Interval &old_interval_input,
                                 const Interval &new_interval_input,
                                 QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input),
      old_interval(old_interval_input), new_interval(new_interval_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetChordInterval::undo() {
  chords_model_pointer->set_chord_interval(chord_number,
                                          old_interval);
}

void SetChordInterval::redo() {
  chords_model_pointer->set_chord_interval(chord_number,
                                          new_interval);
}
