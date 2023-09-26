#pragma once

#include <qundostack.h>  // for QUndoCommand

#include <memory>  // for unique_ptr
#include <vector>  // for vector

#include "main/TreeNode.h"          // for TreeNode
#include "utilities/StableIndex.h"  // for StableIndex

class Editor;  // lines 12-12
class QModelIndex;

class RemoveChange : public QUndoCommand {
 public:
  Editor* editor_pointer;
  int first_index;
  int number_of_children;
  StableIndex stable_parent_index;
  std::vector<std::unique_ptr<TreeNode>> deleted_children;

  explicit RemoveChange(Editor* editor_pointer_input, int first_index_input,
                        int number_of_rows_input,
                        const QModelIndex &parent_index_input,
                        QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
