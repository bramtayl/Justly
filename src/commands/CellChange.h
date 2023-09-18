#pragma once

#include <qundostack.h>  // for QUndoCommand
#include <qvariant.h>    // for QVariant

#include "StableIndex.h"  // for StableIndex

class Editor;  // lines 12-12
class QModelIndex;

class CellChange : public QUndoCommand {
 public:
  Editor &editor;
  const StableIndex stable_parent_index;
  const QVariant old_value;
  const QVariant new_value;
  explicit CellChange(Editor &editor_input, const QModelIndex &parent_index_input,
                   QVariant new_value_input,
                   QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
