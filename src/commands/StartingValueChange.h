#pragma once

#include <qundostack.h>  // for QUndoCommand
#include <qvariant.h>

#include <gsl/pointers>

#include "main/Song.h"

class Editor;  // lines 12-12

class StartingValueChange : public QUndoCommand {
 private:
  gsl::not_null<Editor *> editor_pointer;
  StartingFieldId value_type;
  QVariant old_value;
  QVariant new_value;

 public:
  explicit StartingValueChange(gsl::not_null<Editor *>,
                               StartingFieldId,
                               QVariant,
                               QVariant);
  void undo() override;
  void redo() override;
  [[nodiscard]] auto id() const -> int override;
  auto mergeWith(const QUndoCommand * next_command_pointer) -> bool override;
};
