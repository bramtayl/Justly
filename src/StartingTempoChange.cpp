#include "src/StartingTempoChange.h"

#include "justly/SongEditor.h"     // for SongEditor
#include "src/StartingField.h"  // for starting_tempo_id

StartingTempoChange::StartingTempoChange(SongEditor *editor_pointer_input,
                                         double old_value_input,
                                         double new_value_input)
    : editor_pointer(editor_pointer_input),
      old_value(old_value_input),
      new_value(new_value_input) {}

auto StartingTempoChange::id() const -> int { return starting_tempo_id; }

auto StartingTempoChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  new_value = dynamic_cast<const StartingTempoChange *>(next_command_pointer)
                  ->new_value;
  return true;
}

void StartingTempoChange::redo() {
  editor_pointer->set_starting_tempo(new_value);
}

void StartingTempoChange::undo() {
  editor_pointer->set_starting_tempo(old_value);
}
