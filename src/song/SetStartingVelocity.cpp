#include "song/SetStartingVelocity.hpp"

#include <QtGlobal>

#include "other/CommandId.hpp"
#include "song/SongEditor.hpp"

SetStartingVelocity::SetStartingVelocity(SongEditor *song_editor_pointer_input,
                                         double old_value_input,
                                         double new_value_input)
    : song_editor_pointer(song_editor_pointer_input),
      old_value(old_value_input), new_value(new_value_input) {
  Q_ASSERT(song_editor_pointer != nullptr);
};

auto SetStartingVelocity::id() const -> int { return set_starting_velocity_id; }

auto SetStartingVelocity::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  Q_ASSERT(next_command_pointer != nullptr);

  const auto *next_velocity_change_pointer =
      dynamic_cast<const SetStartingVelocity *>(next_command_pointer);

  Q_ASSERT(next_velocity_change_pointer != nullptr);
  new_value = next_velocity_change_pointer->new_value;
  return true;
}

void SetStartingVelocity::undo() {
  song_editor_pointer->set_starting_velocity_directly(old_value);
}

void SetStartingVelocity::redo() {
  song_editor_pointer->set_starting_velocity_directly(new_value);
}
