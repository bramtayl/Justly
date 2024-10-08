#include "song/SetStartingKey.hpp"

#include <QtGlobal>

#include "other/CommandId.hpp"
#include "song/SongEditor.hpp"

SetStartingKey::SetStartingKey(SongEditor *song_editor_pointer_input,
                               double old_value_input, double new_value_input)
    : song_editor_pointer(song_editor_pointer_input),
      old_value(old_value_input), new_value(new_value_input) {
  Q_ASSERT(song_editor_pointer != nullptr);
};

auto SetStartingKey::id() const -> int { return set_starting_key_id; }

auto SetStartingKey::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  Q_ASSERT(next_command_pointer != nullptr);

  const auto *next_tempo_change_pointer =
      dynamic_cast<const SetStartingKey *>(next_command_pointer);

  Q_ASSERT(next_tempo_change_pointer != nullptr);
  new_value = next_tempo_change_pointer->new_value;
  return true;
}

void SetStartingKey::undo() {
  song_editor_pointer->set_starting_key_directly(old_value);
}

void SetStartingKey::redo() {
  song_editor_pointer->set_starting_key_directly(new_value);
}
