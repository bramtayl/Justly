#include "commands/StartingVolumeChange.h"

#include "editors/ShowSlider.h"  // for ShowSlider
#include "main/Editor.h"         // for Editor
#include "main/Song.h"           // for Song
#include "utilities/utilities.h"

StartingVolumeChange::StartingVolumeChange(gsl::not_null<Editor*> editor_pointer_input,
                                           double new_value_input)
    : editor_pointer(editor_pointer_input),
      old_value(editor_pointer_input->song_pointer->starting_volume),
      new_value(new_value_input) {}

auto StartingVolumeChange::id() const -> int {
  return starting_volume_change_id;
}

void StartingVolumeChange::redo() {
  editor_pointer->register_changed();
  if (!first_time) {
    editor_pointer->starting_volume_editor_pointer->set_value_no_signals(new_value);
  }
  editor_pointer->song_pointer->starting_volume = new_value;
  first_time = false;
}

void StartingVolumeChange::undo() {
  editor_pointer->starting_volume_editor_pointer->set_value_no_signals(old_value);
  editor_pointer->song_pointer->starting_volume = old_value;
}

auto StartingVolumeChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  new_value = dynamic_cast<const StartingVolumeChange *>(next_command_pointer)
                  ->new_value;
  return true;
}
