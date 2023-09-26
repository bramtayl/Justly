#pragma once

#include <qundostack.h>  // for QUndoCommand

class Editor;  // lines 12-12

class StartingKeyChange : public QUndoCommand {
 public:
  Editor* editor_pointer;
  double old_value;
  double new_value;
  bool first_time = true;

  explicit StartingKeyChange(Editor* editor_pointer_input, double new_value_input);
  void undo() override;
  void redo() override;
  [[nodiscard]] auto id() const -> int override;
  auto mergeWith(const QUndoCommand *next_command_pointer) -> bool override;
};
