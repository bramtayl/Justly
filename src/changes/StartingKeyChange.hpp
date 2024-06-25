#pragma once

#include <qundostack.h>  // for QUndoCommand

class SongEditor;  // lines 12-12

class StartingKeyChange : public QUndoCommand {
  SongEditor* editor_pointer;
  double old_value;
  double new_value;

 public:
  explicit StartingKeyChange(SongEditor* editor_pointer_input,
                             double old_value_input, double new_value_input);

  [[nodiscard]] auto id() const -> int override;
  [[nodiscard]] auto mergeWith(const QUndoCommand* next_command_pointer) -> bool override;

  void undo() override;
  void redo() override;
};
