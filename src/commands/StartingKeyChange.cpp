#include "commands/StartingKeyChange.h"

#include <qpointer.h>    // for QPointer

#include "Editor.h"       // for Editor
#include "editors/ShowSlider.h"   // for ShowSlider
#include "Song.h"         // for Song
#include "utilities.h"

StartingKeyChange::StartingKeyChange(Editor &editor_input,
                                     double new_value_input)
    : editor(editor_input),
      old_value(editor_input.song.starting_key),
      new_value(new_value_input) {}

// set frequency will emit a signal to update the slider
void StartingKeyChange::redo() {
  editor.register_changed();
  if (!first_time) {
    editor.starting_key_show_slider_pointer->set_value_no_signals(new_value);
  }
  editor.song.starting_key = new_value;
  if (first_time) {
    first_time = false;
  }
}

void StartingKeyChange::undo() {
  editor.starting_key_show_slider_pointer->set_value_no_signals(old_value);
  editor.song.starting_key = old_value;
}

auto StartingKeyChange::id() const -> int { return starting_key_change_id; }

auto StartingKeyChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  new_value =
      dynamic_cast<const StartingKeyChange *>(next_command_pointer)->new_value;
  return true;
}
