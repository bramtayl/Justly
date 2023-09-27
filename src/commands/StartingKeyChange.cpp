#include "commands/StartingKeyChange.h"

#include "editors/ShowSlider.h"  // for ShowSlider
#include "main/Editor.h"         // for Editor
#include "main/Song.h"           // for Song
#include "utilities/utilities.h"

StartingKeyChange::StartingKeyChange(Editor* editor_pointer_input,
                                     double new_value_input)
    : editor_pointer(editor_pointer_input),
      old_value(editor_pointer_input->song_pointer->starting_key),
      new_value(new_value_input) {}

// set frequency will emit a signal to update the slider
void StartingKeyChange::redo() {
  editor_pointer->register_changed();
  if (!first_time) {
    editor_pointer->starting_key_editor_pointer->set_value_no_signals(new_value);
  }
  editor_pointer->song_pointer->starting_key = new_value;
  first_time = false;
}

void StartingKeyChange::undo() {
  editor_pointer->starting_key_editor_pointer->set_value_no_signals(old_value);
  editor_pointer->song_pointer->starting_key = old_value;
}

auto StartingKeyChange::id() const -> int { return starting_key_change_id; }

auto StartingKeyChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  new_value =
      dynamic_cast<const StartingKeyChange *>(next_command_pointer)->new_value;
  return true;
}
