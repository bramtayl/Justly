#include "changes/GainChange.hpp"

#include <QtGlobal>

#include "changes/ChangeId.hpp"
#include "justly/SongEditor.hpp"

GainChange::GainChange(SongEditor *song_editor_pointer_input,
                           int old_value_input,
                           int new_value_input)
    : song_editor_pointer(song_editor_pointer_input),
      old_value(old_value_input),
      new_value(new_value_input){};

auto GainChange::id() const -> int { return gain_id; }

auto GainChange::mergeWith(const QUndoCommand *next_command_pointer) -> bool {
  Q_ASSERT(next_command_pointer != nullptr);

  const auto *next_gain_change_pointer =
      dynamic_cast<const GainChange *>(next_command_pointer);

  Q_ASSERT(next_gain_change_pointer != nullptr);
  new_value = next_gain_change_pointer->new_value;
  return true;
}

void GainChange::undo() {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->set_gain_percent_directly(old_value);
}

void GainChange::redo() {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->set_gain_percent_directly(new_value);
}
