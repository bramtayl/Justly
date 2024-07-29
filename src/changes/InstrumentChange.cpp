#include "changes/InstrumentChange.hpp"

#include <QtGlobal>

#include "changes/ChangeId.hpp"
#include "justly/SongEditor.hpp"

struct Instrument;

InstrumentChange::InstrumentChange(SongEditor *song_editor_pointer_input,
                                   const Instrument *old_value_input,
                                   const Instrument *new_value_input)
    : song_editor_pointer(song_editor_pointer_input),
      old_value(old_value_input), new_value(new_value_input) {}

auto InstrumentChange::id() const -> int { return starting_instrument_id; }

auto InstrumentChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  Q_ASSERT(next_command_pointer != nullptr);
  const auto *next_instrument_change_pointer =
      dynamic_cast<const InstrumentChange *>(next_command_pointer);

  Q_ASSERT(next_instrument_change_pointer != nullptr);
  new_value = next_instrument_change_pointer->new_value;
  return true;
}

void InstrumentChange::undo() {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->set_instrument_directly(old_value);
}

void InstrumentChange::redo() {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->set_instrument_directly(new_value);
}
