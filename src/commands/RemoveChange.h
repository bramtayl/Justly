#pragma once

#include <qundostack.h>  // for QUndoCommand

#include <memory>  // for unique_ptr
#include <vector>  // for vector

#include "StableIndex.h"  // for StableIndex
#include "TreeNode.h"     // for TreeNode

class Editor;  // lines 12-12
class QModelIndex;

class RemoveChange : public QUndoCommand {
 public:
  Editor &editor;
  const int first_index;
  const int number_of_children;
  const StableIndex stable_parent_index;
  std::vector<std::unique_ptr<TreeNode>> deleted_children;

  explicit RemoveChange(Editor &editor_input, int first_index_input,
                        int number_of_rows_input,
                        const QModelIndex &parent_index_input,
                        QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
