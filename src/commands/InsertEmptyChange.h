#pragma once

#include <qundostack.h>  // for QUndoCommand

#include <gsl/pointers>

#include "utilities/StableIndex.h"  // for StableIndex

class ChordsModel;  // lines 12-12

class InsertEmptyChange : public QUndoCommand {
 private:
  gsl::not_null<ChordsModel *> chords_model_pointer;
  int first_child_number;
  int number_of_children;
  StableIndex stable_parent_index;

 public:
  explicit InsertEmptyChange(
      gsl::not_null<ChordsModel *> chords_model_pointer_input,
      int first_child_number_input, int number_of_children_input,
      const StableIndex &stable_parent_index_input,
      QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
