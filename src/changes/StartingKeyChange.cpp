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
  Q_ASSERT(next_command_pointer != nullptr);
  const auto *next_key_change_pointer =
      dynamic_cast<const StartingKeyChange *>(next_command_pointer);
  
  Q_ASSERT(next_key_change_pointer != nullptr);
  new_value = next_key_change_pointer->new_value;
  return true;
}

void StartingKeyChange::undo() {
  Q_ASSERT(editor_pointer != nullptr);
  editor_pointer->set_starting_key_directly(old_value);
}

void StartingKeyChange::redo() {
  Q_ASSERT(editor_pointer != nullptr);
  editor_pointer->set_starting_key_directly(new_value);
}
