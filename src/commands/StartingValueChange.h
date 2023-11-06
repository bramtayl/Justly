#pragma once

#include <qundostack.h>  // for QUndoCommand
#include <qvariant.h>

#include "justly/Song.h"

class SongEditor;  // lines 12-12

class StartingValueChange : public QUndoCommand {
  SongEditor* editor_pointer;
  StartingFieldId value_type;
  QVariant old_value;
  QVariant new_value;

 public:
  explicit StartingValueChange(SongEditor*, StartingFieldId, QVariant,
                               QVariant);
  void undo() override;
  void redo() override;
  [[nodiscard]] auto id() const -> int override;
  auto mergeWith(const QUndoCommand* next_command_pointer) -> bool override;
};
