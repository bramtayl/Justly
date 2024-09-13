#include "song/SetGain.hpp"

#include <QtGlobal>

#include "other/CommandId.hpp"
#include "song/SongEditor.hpp"

SetGain::SetGain(SongEditor *song_editor_pointer_input, double old_value_input,
                 double new_value_input)
    : song_editor_pointer(song_editor_pointer_input),
      old_value(old_value_input), new_value(new_value_input) {
  Q_ASSERT(song_editor_pointer != nullptr);
};

auto SetGain::id() const -> int { return set_gain_id; }

auto SetGain::mergeWith(const QUndoCommand *next_command_pointer) -> bool {
  Q_ASSERT(next_command_pointer != nullptr);

  const auto *next_gain_change_pointer =
      dynamic_cast<const SetGain *>(next_command_pointer);

  Q_ASSERT(next_gain_change_pointer != nullptr);
  new_value = next_gain_change_pointer->new_value;
  return true;
}

void SetGain::undo() { song_editor_pointer->set_gain_directly(old_value); }

void SetGain::redo() { song_editor_pointer->set_gain_directly(new_value); }
