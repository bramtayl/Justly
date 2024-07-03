#pragma once

#include <qundostack.h>  // for QUndoCommand

#include "changes/ChangeId.hpp"  // for ChangeId

class QDoubleSpinBox;

class DoubleChange : public QUndoCommand {
  QDoubleSpinBox* const spinbox_pointer;
  const ChangeId change_id;
  const double old_value;
  double new_value;

 public:
  explicit DoubleChange(QDoubleSpinBox* spinbox_pointer_input,
                                 ChangeId change_id_input,
                                 double old_value_input,
                                 double new_value_input);

  [[nodiscard]] auto id() const -> int override;
  [[nodiscard]] auto mergeWith(const QUndoCommand* next_command_pointer)
      -> bool override;

  void undo() override;
  void redo() override;

  void set_starting_value_directly(double new_value);
};
