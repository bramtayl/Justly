#include "changes/VelocityChange.hpp"

#include <QtGlobal>

#include "changes/ChangeId.hpp"
#include "justly/SongEditor.hpp"

VelocityChange::VelocityChange(SongEditor *song_editor_pointer_input,
                           double old_value_input,
                           double new_value_input)
    : song_editor_pointer(song_editor_pointer_input),
      old_value(old_value_input),
      new_value(new_value_input){};

auto VelocityChange::id() const -> int { return starting_velocity_id; }

auto VelocityChange::mergeWith(const QUndoCommand *next_command_pointer) -> bool {
  Q_ASSERT(next_command_pointer != nullptr);

  const auto *next_velocity_change_pointer =
      dynamic_cast<const VelocityChange *>(next_command_pointer);

  Q_ASSERT(next_velocity_change_pointer != nullptr);
  new_value = next_velocity_change_pointer->new_value;
  return true;
}

void VelocityChange::undo() {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->set_starting_velocity_directly(old_value);
}

void VelocityChange::redo() {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->set_starting_velocity_directly(new_value);
}
