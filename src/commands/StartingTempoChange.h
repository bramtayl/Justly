#pragma once

#include <qundostack.h>  // for QUndoCommand

class Editor;  // lines 12-12

class StartingTempoChange : public QUndoCommand {
 public:
  Editor &editor;
  const double old_value;
  double new_value;
  bool first_time = true;

  explicit StartingTempoChange(Editor &editor_input, double new_value);
  void undo() override;
  void redo() override;
  [[nodiscard]] auto id() const -> int override;
  auto mergeWith(const QUndoCommand *next_command_pointer) -> bool override;
};
