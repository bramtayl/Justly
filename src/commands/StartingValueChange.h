#pragma once

#include <qundostack.h>  // for QUndoCommand
#include <qvariant.h>

#include <gsl/pointers>

#include "utilities/utilities.h"

class Editor;  // lines 12-12

class StartingValueChange : public QUndoCommand {
 private:
  gsl::not_null<Editor *> editor_pointer;
  StartingFieldId value_type;
  QVariant old_value;
  QVariant new_value;
  bool first_time = true;

 public:
  explicit StartingValueChange(gsl::not_null<Editor *> editor_pointer_input,
                               StartingFieldId value_type_input,
                               QVariant old_value_input,
                               QVariant new_value_input);
  void undo() override;
  void redo() override;
  [[nodiscard]] auto id() const -> int override;
  auto mergeWith(const QUndoCommand *next_command_pointer) -> bool override;
};
