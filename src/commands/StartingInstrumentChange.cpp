#include "commands/StartingInstrumentChange.h"

#include <utility>  // for move

#include "main/Editor.h"  // for Editor
#include "main/Song.h"    // for Song
#include "utilities/utilities.h"

StartingInstrumentChange::StartingInstrumentChange(
    gsl::not_null<Editor*>editor_pointer_input, Instrument new_starting_instrument_input)
    : editor_pointer(editor_pointer_input),
      old_starting_instrument(editor_pointer_input->song_pointer->starting_instrument),
      new_starting_instrument(std::move(new_starting_instrument_input)) {}

void StartingInstrumentChange::undo() {
  editor_pointer->set_starting_instrument(old_starting_instrument, true);
}

void StartingInstrumentChange::redo() {
  editor_pointer->register_changed();
  first_time = false;
  editor_pointer->set_starting_instrument(new_starting_instrument, !first_time);
}

auto StartingInstrumentChange::id() const -> int {
  return starting_instrument_change_id;
}

auto StartingInstrumentChange::mergeWith(
    const QUndoCommand *next_command_pointer) -> bool {
  new_starting_instrument =
      dynamic_cast<const StartingInstrumentChange *>(next_command_pointer)
          ->new_starting_instrument;
  return true;
}
