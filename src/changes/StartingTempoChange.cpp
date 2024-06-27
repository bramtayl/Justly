#include "changes/StartingTempoChange.hpp"

#include "changes/ChangeId.hpp"   // for starting_tempo_id
#include "justly/SongEditor.hpp"  // for SongEditor

StartingTempoChange::StartingTempoChange(SongEditor *editor_pointer_input,
                                         double old_value_input,
                                         double new_value_input)
    : editor_pointer(editor_pointer_input),
      old_value(old_value_input),
      new_value(new_value_input) {}

auto StartingTempoChange::id() const -> int { return starting_tempo_id; }

auto StartingTempoChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  Q_ASSERT(next_command_pointer != nullptr);
  
  const auto *next_tempo_change_pointer =
      dynamic_cast<const StartingTempoChange *>(next_command_pointer);
      
  Q_ASSERT(next_tempo_change_pointer != nullptr);
  new_value = next_tempo_change_pointer->new_value;
  return true;
}

void StartingTempoChange::undo() {
  Q_ASSERT(editor_pointer != nullptr);
  editor_pointer->set_starting_tempo_directly(old_value);
}

void StartingTempoChange::redo() {
  Q_ASSERT(editor_pointer != nullptr);
  editor_pointer->set_starting_tempo_directly(new_value);
}
