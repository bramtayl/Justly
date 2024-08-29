#include "commands/ChangeToInterval.hpp"

#include <QtGlobal>

#include "other/ChordsModel.hpp"

ChangeToInterval::ChangeToInterval(
    ChordsModel *chords_model_pointer_input, size_t chord_number_input,
    size_t note_number_input, const Instrument *old_instrument_pointer_input,
    const Instrument *new_instrument_pointer_input,
    const Percussion *old_percussion_pointer_input,
    const Interval &new_interval_input, QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), note_number(note_number_input),
      old_instrument_pointer(old_instrument_pointer_input),
      new_instrument_pointer(new_instrument_pointer_input),
      old_percussion_pointer(old_percussion_pointer_input),
      new_interval(new_interval_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void ChangeToInterval::undo() {
  change_to_percussion(chords_model_pointer, chord_number, note_number,
                       old_instrument_pointer, old_percussion_pointer);
}

void ChangeToInterval::redo() {
  change_to_interval(chords_model_pointer, chord_number, note_number,
                     new_instrument_pointer, new_interval);
}
