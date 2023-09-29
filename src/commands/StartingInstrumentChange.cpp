#include "commands/StartingInstrumentChange.h"

#include "main/Editor.h"          // for Editor
#include "main/Song.h"            // for Song
#include "utilities/utilities.h"  // for starting_instrument_change_id

class Instrument;

StartingInstrumentChange::StartingInstrumentChange(
    gsl::not_null<Editor*> editor_pointer_input,
    const Instrument& new_starting_instrument_input)
    : editor_pointer(editor_pointer_input),
      old_starting_instrument_pointer(
          &(editor_pointer_input->song_pointer->get_starting_instrument())),
      new_starting_instrument_pointer(&new_starting_instrument_input) {}

void StartingInstrumentChange::undo() {
  editor_pointer->set_starting_instrument(get_old_starting_instrument(), true);
}

void StartingInstrumentChange::redo() {
  editor_pointer->register_changed();
  first_time = false;
  editor_pointer->set_starting_instrument(get_new_starting_instrument(),
                                          !first_time);
}

auto StartingInstrumentChange::id() const -> int {
  return starting_instrument_change_id;
}

auto StartingInstrumentChange::mergeWith(
    const QUndoCommand* next_command_pointer) -> bool {
  set_new_starting_instrument(
      dynamic_cast<const StartingInstrumentChange*>(next_command_pointer)
          ->get_new_starting_instrument());
  return true;
}

auto StartingInstrumentChange::get_old_starting_instrument() const
    -> const Instrument& {
  return *old_starting_instrument_pointer;
}

auto StartingInstrumentChange::get_new_starting_instrument() const
    -> const Instrument& {
  return *new_starting_instrument_pointer;
}

void StartingInstrumentChange::set_old_starting_instrument(
    const Instrument& new_instrument) {
  old_starting_instrument_pointer = &new_instrument;
};

void StartingInstrumentChange::set_new_starting_instrument(
    const Instrument& new_instrument) {
  new_starting_instrument_pointer = &new_instrument;
};