#include "commands/SetStartingInstrument.hpp"

#include <QtGlobal>

#include "commands/CommandId.hpp"
#include "other/SongEditor.hpp"

struct Instrument;

SetStartingInstrument::SetStartingInstrument(
    SongEditor *song_editor_pointer_input, const Instrument *old_value_input,
    const Instrument *new_value_input)
    : song_editor_pointer(song_editor_pointer_input),
      old_value(old_value_input), new_value(new_value_input) {
  Q_ASSERT(song_editor_pointer != nullptr);
}

auto SetStartingInstrument::id() const -> int {
  return set_starting_instrument_id;
}

auto SetStartingInstrument::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  Q_ASSERT(next_command_pointer != nullptr);
  const auto *next_instrument_change_pointer =
      dynamic_cast<const SetStartingInstrument *>(next_command_pointer);

  Q_ASSERT(next_instrument_change_pointer != nullptr);
  new_value = next_instrument_change_pointer->new_value;
  return true;
}

void SetStartingInstrument::undo() {
  set_starting_instrument_directly(song_editor_pointer, old_value);
}

void SetStartingInstrument::redo() {
  set_starting_instrument_directly(song_editor_pointer, new_value);
}
