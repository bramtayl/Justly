#include "song/SetStartingDouble.hpp"

#include <QtGlobal>

#include "song/ControlId.hpp"
#include "song/Song.hpp"
#include "song/SongEditor.hpp"

SetStartingDouble::SetStartingDouble(SongEditor& song_editor,
                                     ControlId command_id_input,
                                     double new_value_input)
    : song_editor_pointer(&song_editor),
      command_id(command_id_input),
      old_value(
          get_double(song_editor.song, command_id)),
      new_value(new_value_input) {
  Q_ASSERT(song_editor_pointer != nullptr);
};

auto SetStartingDouble::id() const -> int { return starting_velocity_id; }

auto SetStartingDouble::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  Q_ASSERT(next_command_pointer != nullptr);

  const auto *next_velocity_change_pointer =
      dynamic_cast<const SetStartingDouble *>(next_command_pointer);

  Q_ASSERT(next_velocity_change_pointer != nullptr);
  new_value = next_velocity_change_pointer->new_value;
  return true;
}

void SetStartingDouble::undo() {
  set_double_directly(*song_editor_pointer, command_id, old_value);
}

void SetStartingDouble::redo() {
  set_double_directly(*song_editor_pointer, command_id, new_value);
}
