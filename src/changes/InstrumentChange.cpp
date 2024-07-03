#include "changes/InstrumentChange.hpp"

#include <qassert.h>  // for Q_ASSERT

#include "changes/ChangeId.hpp"         // for starting_instrument_id
#include "justly/InstrumentEditor.hpp"  // for InstrumentEditor

struct Instrument;

InstrumentChange::InstrumentChange(
    InstrumentEditor *starting_instrument_editor_pointer_input,
    const Instrument *old_value_input, const Instrument *new_value_input)
    : starting_instrument_editor_pointer(
          starting_instrument_editor_pointer_input),
      old_value(old_value_input),
      new_value(new_value_input) {}

auto InstrumentChange::id() const -> int {
  return starting_instrument_id;
}

auto InstrumentChange::mergeWith(
    const QUndoCommand *next_command_pointer) -> bool {
  Q_ASSERT(next_command_pointer != nullptr);
  const auto *next_instrument_change_pointer =
      dynamic_cast<const InstrumentChange *>(next_command_pointer);

  Q_ASSERT(next_instrument_change_pointer != nullptr);
  new_value = next_instrument_change_pointer->new_value;
  return true;
}

void InstrumentChange::undo() {
  set_starting_instrument_directly(old_value);
}

void InstrumentChange::redo() {
  set_starting_instrument_directly(new_value);
}

void InstrumentChange::set_starting_instrument_directly(
    const Instrument *new_value) {
  Q_ASSERT(starting_instrument_editor_pointer != nullptr);
  if (starting_instrument_editor_pointer->value() != new_value) {
    starting_instrument_editor_pointer->blockSignals(true);
    starting_instrument_editor_pointer->setValue(new_value);
    starting_instrument_editor_pointer->blockSignals(false);
  }
}