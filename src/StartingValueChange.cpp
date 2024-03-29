#include "src/StartingValueChange.h"

#include <utility>

#include "justly/SongEditor.h"  // for SongEditor
#include "justly/StartingFieldId.h"

StartingValueChange::StartingValueChange(SongEditor *editor_pointer_input,
                                         StartingFieldId value_type_input,
                                         QVariant old_value_input,
                                         QVariant new_value_input)
    : editor_pointer(editor_pointer_input),
      value_type(value_type_input),
      old_value(std::move(old_value_input)),
      new_value(std::move(new_value_input)) {}

auto StartingValueChange::id() const -> int { return value_type; }

auto StartingValueChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  new_value = dynamic_cast<const StartingValueChange *>(next_command_pointer)
                  ->new_value;
  return true;
}

void StartingValueChange::redo() {
  editor_pointer->set_starting_control(value_type, new_value, true);
}

void StartingValueChange::undo() {
  editor_pointer->set_starting_control(value_type, old_value, true);
}
