#include "song/SetStartingDouble.hpp"

#include <QtGlobal>

#include "song/ControlId.hpp"
#include "song/Song.hpp"
#include "song/SongEditor.hpp"

void set_starting_double(SetStartingDouble &change, bool is_new) {
  set_double_directly(change.song_editor, change.command_id,
                      is_new ? change.new_value : change.old_value);
}

SetStartingDouble::SetStartingDouble(SongEditor &song_editor_input,
                                     ControlId command_id_input,
                                     double new_value_input)
    : song_editor(song_editor_input), command_id(command_id_input),
      old_value(get_double(song_editor_input.song, command_id)),
      new_value(new_value_input){};

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
  set_starting_double(*this, false);
}

void SetStartingDouble::redo() {
  set_starting_double(*this, true);
}
