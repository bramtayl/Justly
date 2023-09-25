#include "commands/StartingInstrumentChange.h"

#include <utility>  // for move

#include "Editor.h"  // for Editor
#include "Song.h"    // for Song
#include "utilities.h"

StartingInstrumentChange::StartingInstrumentChange(
    Editor &editor_input, Instrument new_starting_instrument_input)
    : editor(editor_input),
      old_starting_instrument(editor_input.song.starting_instrument),
      new_starting_instrument(std::move(new_starting_instrument_input)) {}

void StartingInstrumentChange::undo() {
  editor.set_starting_instrument(old_starting_instrument, true);
}

void StartingInstrumentChange::redo() {
  editor.register_changed();
  if (first_time) {
    first_time = false;
  }
  editor.set_starting_instrument(new_starting_instrument, !first_time);
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