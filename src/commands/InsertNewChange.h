#pragma once

#include <qundostack.h>  // for QUndoCommand

#include "utilities/StableIndex.h"  // for StableIndex

class Editor;  // lines 12-12
class QModelIndex;

class InsertNewChange : public QUndoCommand {
 private:
  Editor* editor_pointer;
  int first_index;
  int number_of_children;
  StableIndex stable_parent_index;
 public:
  explicit InsertNewChange(Editor* editor_pointer_input, int first_index_input,
                           int number_of_rows_input,
                           const QModelIndex &parent_index_input,
                           QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
