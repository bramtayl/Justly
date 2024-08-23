#include "commands/SetStartingTempo.hpp"

#include <QtGlobal>

#include "commands/CommandId.hpp"
#include "other/SongEditor.hpp"

SetStartingTempo::SetStartingTempo(SongEditor *song_editor_pointer_input,
                                   double old_value_input,
                                   double new_value_input)
    : song_editor_pointer(song_editor_pointer_input),
      old_value(old_value_input), new_value(new_value_input){
  Q_ASSERT(song_editor_pointer_input != nullptr);
};

auto SetStartingTempo::id() const -> int { return set_starting_tempo_id; }

auto SetStartingTempo::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  Q_ASSERT(next_command_pointer != nullptr);

  const auto *next_tempo_change_pointer =
      dynamic_cast<const SetStartingTempo *>(next_command_pointer);

  Q_ASSERT(next_tempo_change_pointer != nullptr);
  new_value = next_tempo_change_pointer->new_value;
  return true;
}

void SetStartingTempo::undo() {
  set_starting_tempo_directly(song_editor_pointer, old_value);
}

void SetStartingTempo::redo() {
  set_starting_tempo_directly(song_editor_pointer, new_value);
}
