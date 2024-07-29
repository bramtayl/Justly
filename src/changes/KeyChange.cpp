#include "changes/KeyChange.hpp"

#include <QtGlobal>

#include "changes/ChangeId.hpp"
#include "justly/SongEditor.hpp"

KeyChange::KeyChange(SongEditor *song_editor_pointer_input,
                           double old_value_input,
                           double new_value_input)
    : song_editor_pointer(song_editor_pointer_input),
      old_value(old_value_input),
      new_value(new_value_input){};

auto KeyChange::id() const -> int { return starting_key_id; }

auto KeyChange::mergeWith(const QUndoCommand *next_command_pointer) -> bool {
  Q_ASSERT(next_command_pointer != nullptr);

  const auto *next_tempo_change_pointer =
      dynamic_cast<const KeyChange *>(next_command_pointer);

  Q_ASSERT(next_tempo_change_pointer != nullptr);
  new_value = next_tempo_change_pointer->new_value;
  return true;
}

void KeyChange::undo() {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->set_key_directly(old_value);
}

void KeyChange::redo() {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->set_key_directly(new_value);
}
