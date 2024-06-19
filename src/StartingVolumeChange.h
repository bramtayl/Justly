#pragma once

#include <qundostack.h>  // for QUndoCommand

#include "justly/global.h"

class SongEditor;  // lines 12-12

class JUSTLY_EXPORT StartingVolumeChange : public QUndoCommand {
  SongEditor* editor_pointer;
  double old_value;
  double new_value;

 public:
  explicit StartingVolumeChange(SongEditor*, double, double);
  void undo() override;
  void redo() override;
  [[nodiscard]] auto id() const -> int override;
  auto mergeWith(const QUndoCommand* next_command_pointer) -> bool override;
};
