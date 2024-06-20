#include "src/InsertEmptyChange.h"

#include "src/ChordsModel.h"  // for ChordsModel

InsertEmptyChange::InsertEmptyChange(ChordsModel* chords_model_pointer_input,
                                     int first_child_number_input,
                                     int number_of_children_input,
                                     int parent_number_input,
                                     QUndoCommand* parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_child_number(first_child_number_input),
      number_of_children(number_of_children_input),
      parent_number(parent_number_input) {}

void InsertEmptyChange::undo() {
  chords_model_pointer->remove(
      first_child_number, number_of_children, parent_number);
}

void InsertEmptyChange::redo() {
  chords_model_pointer->insert_empty(
      first_child_number, number_of_children, parent_number);
}

