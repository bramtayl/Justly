#pragma once

#include <qundostack.h>  // for QUndoCommand

#include <nlohmann/json.hpp>      // for basic_json
#include <nlohmann/json_fwd.hpp>  // for json

#include "utilities/StableIndex.h"  // for StableIndex

class Editor;  // lines 12-12
class QModelIndex;

class InsertChange : public QUndoCommand {
 private:
  Editor* editor_pointer;
  int first_index;
  nlohmann::json insertion;
  StableIndex stable_parent_index;
 public:
  InsertChange(Editor* editor_pointer_input, int first_index_input,
               nlohmann::json insertion_input,
               const QModelIndex &parent_index_input,
               QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
