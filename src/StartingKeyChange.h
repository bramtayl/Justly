#pragma once

#include <qundostack.h>  // for QUndoCommand

class SongEditor;  // lines 12-12

class StartingKeyChange : public QUndoCommand {
  SongEditor* editor_pointer;
  double old_value;
  double new_value;

 public:
  explicit StartingKeyChange(SongEditor*, double, double);
  void undo() override;
  void redo() override;
  [[nodiscard]] auto id() const -> int override;
  auto mergeWith(const QUndoCommand* next_command_pointer) -> bool override;
};