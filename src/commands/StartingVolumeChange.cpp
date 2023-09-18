#include "commands/StartingVolumeChange.h"

#include <qpointer.h>    // for QPointer

#include "Editor.h"       // for Editor
#include "editors/ShowSlider.h"   // for ShowSlider
#include "Song.h"         // for Song
#include "utilities.h"

StartingVolumeChange::StartingVolumeChange(Editor &editor_input,
                                           double new_value_input)
    : editor(editor_input),
      old_value(editor_input.song.starting_volume),
      new_value(new_value_input) {}

auto StartingVolumeChange::id() const -> int {
  return starting_volume_change_id;
}

void StartingVolumeChange::redo() {
  editor.register_changed();
  if (!first_time) {
    editor.starting_volume_show_slider_pointer->set_value_no_signals(new_value);
  }
  editor.song.starting_volume = new_value;
  if (first_time) {
    first_time = false;
  }
}

void StartingVolumeChange::undo() {
  editor.starting_volume_show_slider_pointer->set_value_no_signals(old_value);
  editor.song.starting_volume = old_value;
}

auto StartingVolumeChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  new_value = dynamic_cast<const StartingVolumeChange *>(next_command_pointer)
                  ->new_value;
  return true;
}
