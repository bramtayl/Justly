#pragma once

#include <qundostack.h>  // for QUndoCommand

class ChordsModel;  // lines 12-12

class InsertEmptyChange : public QUndoCommand {
  ChordsModel* chords_model_pointer;
  int first_child_number;
  int number_of_children;
  int parent_number;

 public:
  explicit InsertEmptyChange(ChordsModel* chords_model_pointer_input,
                             int first_child_number_input,
                             int number_of_children_input,
                             int parent_number_input,
                             QUndoCommand* parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
