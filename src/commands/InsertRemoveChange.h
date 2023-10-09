#pragma once

#include <qundostack.h>  // for QUndoCommand

#include <gsl/pointers>
#include <nlohmann/json.hpp>      // for basic_json
#include <nlohmann/json_fwd.hpp>  // for json

#include "utilities/StableIndex.h"  // for StableIndex

class ChordsModel;  // lines 12-12

class InsertRemoveChange : public QUndoCommand {
 private:
  gsl::not_null<ChordsModel *> chords_model_pointer;
  int first_child_number;
  nlohmann::json insertion;
  StableIndex stable_parent_index;
  bool is_insert;

 public:
  InsertRemoveChange(gsl::not_null<ChordsModel *> chords_model_pointer_input,
               int first_child_number_input, nlohmann::json insertion_input,
               const StableIndex &stable_parent_index_input,
               bool is_insert_input,
               QUndoCommand *parent_pointer_input = nullptr);
  void insert_if(bool should_insert);

  void undo() override;
  void redo() override;
};
