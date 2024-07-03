#include "changes/DoubleChange.hpp"

#include <qassert.h>   // for Q_ASSERT
#include <qspinbox.h>  // for QDoubleSpinBox

#include "changes/ChangeId.hpp"  // for ChangeId

DoubleChange::DoubleChange(
    QDoubleSpinBox *spinbox_pointer_input, ChangeId change_id_input,
    double old_value_input, double new_value_input)
    : spinbox_pointer(spinbox_pointer_input),
      change_id(change_id_input),
      old_value(old_value_input),
      new_value(new_value_input){};

auto DoubleChange::id() const -> int { return change_id; }

auto DoubleChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  Q_ASSERT(next_command_pointer != nullptr);

  const auto *next_tempo_change_pointer =
      dynamic_cast<const DoubleChange *>(next_command_pointer);

  Q_ASSERT(next_tempo_change_pointer != nullptr);
  new_value = next_tempo_change_pointer->new_value;
  return true;
}

void DoubleChange::undo() {
  Q_ASSERT(spinbox_pointer != nullptr);
  set_starting_value_directly(old_value);
}

void DoubleChange::redo() {
  Q_ASSERT(spinbox_pointer != nullptr);
  set_starting_value_directly(new_value);
}

void DoubleChange::set_starting_value_directly(double new_value) {
  Q_ASSERT(spinbox_pointer != nullptr);
  spinbox_pointer->blockSignals(true);
  spinbox_pointer->setValue(new_value);
  spinbox_pointer->blockSignals(false);
}
