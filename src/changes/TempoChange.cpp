#include "changes/TempoChange.hpp"

#include <QtGlobal>

#include "changes/ChangeId.hpp"
#include "justly/SongEditor.hpp"

TempoChange::TempoChange(SongEditor *song_editor_pointer_input,
                           double old_value_input,
                           double new_value_input)
    : song_editor_pointer(song_editor_pointer_input),
      old_value(old_value_input),
      new_value(new_value_input){};

auto TempoChange::id() const -> int { return starting_tempo_id; }

auto TempoChange::mergeWith(const QUndoCommand *next_command_pointer) -> bool {
  Q_ASSERT(next_command_pointer != nullptr);

  const auto *next_tempo_change_pointer =
      dynamic_cast<const TempoChange *>(next_command_pointer);

  Q_ASSERT(next_tempo_change_pointer != nullptr);
  new_value = next_tempo_change_pointer->new_value;
  return true;
}

void TempoChange::undo() {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->set_tempo_directly(old_value);
}

void TempoChange::redo() {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->set_tempo_directly(new_value);
}
