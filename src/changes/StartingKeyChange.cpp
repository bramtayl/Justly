#include "changes/StartingKeyChange.hpp"

#include "changes/ChangeId.hpp"   // for starting_key_id
#include "justly/SongEditor.hpp"  // for SongEditor

StartingKeyChange::StartingKeyChange(SongEditor *editor_pointer_input,
                                     double old_value_input,
                                     double new_value_input)
    : editor_pointer(editor_pointer_input),
      old_value(old_value_input),
      new_value(new_value_input) {}

auto StartingKeyChange::id() const -> int { return starting_key_id; }

auto StartingKeyChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  new_value =
      dynamic_cast<const StartingKeyChange *>(next_command_pointer)->new_value;
  return true;
}

void StartingKeyChange::undo() { editor_pointer->set_starting_key_directly(old_value); }

void StartingKeyChange::redo() { editor_pointer->set_starting_key_directly(new_value); }
