#include "src/StartingVolumeChange.h"

#include "justly/SongEditor.h"     // for SongEditor
#include "src/StartingField.h"  // for starting_volume_id

StartingVolumeChange::StartingVolumeChange(SongEditor *editor_pointer_input,
                                           double old_value_input,
                                           double new_value_input)
    : editor_pointer(editor_pointer_input),
      old_value(old_value_input),
      new_value(new_value_input) {}

auto StartingVolumeChange::id() const -> int { return starting_volume_id; }

auto StartingVolumeChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  new_value = dynamic_cast<const StartingVolumeChange *>(next_command_pointer)
                  ->new_value;
  return true;
}

void StartingVolumeChange::redo() {
  editor_pointer->set_starting_volume(new_value);
}

void StartingVolumeChange::undo() {
  editor_pointer->set_starting_volume(old_value);
}