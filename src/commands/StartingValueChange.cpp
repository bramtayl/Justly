#include "commands/StartingValueChange.h"

#include <utility>

#include "main/Editor.h"  // for Editor

StartingValueChange::StartingValueChange(
    gsl::not_null<Editor *> editor_pointer_input,
    StartingFieldId value_type_input, QVariant old_value_input,
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
  editor_pointer->set_control_no_signals(value_type, new_value);
}

void StartingValueChange::undo() {
  editor_pointer->set_control_no_signals(value_type, old_value);
}
