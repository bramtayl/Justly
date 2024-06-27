#include "changes/StartingVolumeChange.hpp"

#include "changes/ChangeId.hpp"   // for starting_volume_id
#include "justly/SongEditor.hpp"  // for SongEditor

StartingVolumeChange::StartingVolumeChange(SongEditor *editor_pointer_input,
                                           double old_value_input,
                                           double new_value_input)
    : editor_pointer(editor_pointer_input),
      old_value(old_value_input),
      new_value(new_value_input) {}

auto StartingVolumeChange::id() const -> int { return starting_volume_id; }

auto StartingVolumeChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  Q_ASSERT(next_command_pointer != nullptr);
  const auto *next_volume_change_pointer =
      dynamic_cast<const StartingVolumeChange *>(next_command_pointer);
  
  Q_ASSERT(next_volume_change_pointer != nullptr);
  new_value = next_volume_change_pointer->new_value;
  return true;
}

void StartingVolumeChange::undo() {
  Q_ASSERT(editor_pointer != nullptr);
  editor_pointer->set_starting_volume_directly(old_value);
}

void StartingVolumeChange::redo() {
  Q_ASSERT(editor_pointer != nullptr);
  editor_pointer->set_starting_volume_directly(new_value);
}
