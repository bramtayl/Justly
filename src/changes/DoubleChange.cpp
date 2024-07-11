#include "changes/DoubleChange.hpp"

#include <QtGlobal>  // for Q_ASSERT

#include "justly/ChangeId.hpp"  // for ChangeId
#include "justly/SongEditor.hpp"

DoubleChange::DoubleChange(SongEditor *song_editor_pointer_input,
                           ChangeId change_id_input, double old_value_input,
                           double new_value_input)
    : song_editor_pointer(song_editor_pointer_input),
      change_id(change_id_input),
      old_value(old_value_input),
      new_value(new_value_input){};

auto DoubleChange::id() const -> int { return change_id; }

auto DoubleChange::mergeWith(const QUndoCommand *next_command_pointer) -> bool {
  Q_ASSERT(next_command_pointer != nullptr);

  const auto *next_tempo_change_pointer =
      dynamic_cast<const DoubleChange *>(next_command_pointer);

  Q_ASSERT(next_tempo_change_pointer != nullptr);
  new_value = next_tempo_change_pointer->new_value;
  return true;
}

void DoubleChange::undo() {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->set_double_directly(change_id, old_value);
}

void DoubleChange::redo() {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->set_double_directly(change_id, new_value);
}
