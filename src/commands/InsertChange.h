#pragma once

#include <qjsonarray.h>
#include <qundostack.h>  // for QUndoCommand

#include "StableIndex.h"  // for StableIndex

class Editor;  // lines 12-12
class QModelIndex;

class InsertChange : public QUndoCommand {
 public:
  Editor &editor;
  const int first_index;
  const QJsonArray insertion;
  const StableIndex stable_parent_index;

  InsertChange(Editor &editor_input, int first_index_input,
         QJsonArray insertion_input, const QModelIndex &parent_index_input,
         QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
