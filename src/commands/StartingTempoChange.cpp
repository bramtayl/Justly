#include "commands/StartingTempoChange.h"

#include <qpointer.h>    // for QPointer

#include "Editor.h"       // for Editor
#include "editors/ShowSlider.h"   // for ShowSlider
#include "Song.h"         // for Song
#include "utilities.h"

StartingTempoChange::StartingTempoChange(Editor &editor_input,
                                         double new_value_input)
    : editor(editor_input),
      old_value(editor_input.song.starting_tempo),
      new_value(new_value_input) {}

auto StartingTempoChange::id() const -> int { return starting_tempo_change_id; }

auto StartingTempoChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  new_value = dynamic_cast<const StartingTempoChange *>(next_command_pointer)
                  ->new_value;
  return true;
}

void StartingTempoChange::redo() {
  editor.register_changed();
  if (!first_time) {
    editor.starting_tempo_show_slider_pointer->set_value_no_signals(new_value);
  }
  editor.song.starting_tempo = new_value;
  if (first_time) {
    first_time = false;
  }
}

void StartingTempoChange::undo() {
  editor.starting_tempo_show_slider_pointer->set_value_no_signals(old_value);
  editor.song.starting_tempo = old_value;
}
