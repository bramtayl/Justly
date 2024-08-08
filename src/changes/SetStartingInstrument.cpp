#include "changes/SetStartingInstrument.hpp"

#include <QtGlobal>

#include "changes/CommandId.hpp"
#include "justly/SongEditor.hpp"

struct Instrument;

SetStartingInstrument::SetStartingInstrument(
    SongEditor *song_editor_pointer_input, const Instrument *old_value_input,
    const Instrument *new_value_input)
    : song_editor_pointer(song_editor_pointer_input),
      old_value(old_value_input), new_value(new_value_input) {}

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
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->set_starting_instrument_directly(old_value);
}

void SetStartingInstrument::redo() {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->set_starting_instrument_directly(new_value);
}
