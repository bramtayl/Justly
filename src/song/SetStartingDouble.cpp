#include "song/SetStartingDouble.hpp"

#include <QtGlobal>

#include "chord/ChordsModel.hpp"
#include "song/ControlId.hpp"
#include "song/SongEditor.hpp"

static auto get_double(const ChordsModel& chords_model, ControlId command_id) -> double {
  switch (command_id) {
  case gain_id:
    return chords_model.gain;
  case starting_key_id:
    return chords_model.starting_key;
  case starting_velocity_id:
    return chords_model.starting_velocity;
  case starting_tempo_id:
    return chords_model.starting_tempo;
  }
};

SetStartingDouble::SetStartingDouble(SongEditor *song_editor_pointer_input,
                                     ControlId command_id_input,
                                     double new_value_input)
    : song_editor_pointer(song_editor_pointer_input),
      command_id(command_id_input), old_value(get_double(*song_editor_pointer->chords_model_pointer, command_id)),
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
  song_editor_pointer->set_double_directly(command_id, old_value);
}

void SetStartingDouble::redo() {
  song_editor_pointer->set_double_directly(command_id, new_value);
}
