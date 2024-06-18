#include "src/StartingInstrumentChange.h"

#include "justly/SongEditor.h"     // for SongEditor
#include "src/StartingField.h"  // for starting_instrument_id

struct Instrument;

StartingInstrumentChange::StartingInstrumentChange(
    SongEditor *editor_pointer_input, const Instrument *old_value_input,
    const Instrument *new_value_input)
    : editor_pointer(editor_pointer_input),
      old_value(old_value_input),
      new_value(new_value_input) {}

auto StartingInstrumentChange::id() const -> int {
  return starting_instrument_id;
}

auto StartingInstrumentChange::mergeWith(
    const QUndoCommand *next_command_pointer) -> bool {
  new_value =
      dynamic_cast<const StartingInstrumentChange *>(next_command_pointer)
          ->new_value;
  return true;
}

void StartingInstrumentChange::redo() {
  editor_pointer->set_starting_instrument(new_value);
}

void StartingInstrumentChange::undo() {
  editor_pointer->set_starting_instrument(old_value);
}
