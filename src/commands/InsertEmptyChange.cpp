#include "commands/InsertEmptyChange.h"

#include "models/ChordsModel.h"  // for ChordsModel

InsertEmptyChange::InsertEmptyChange(
    gsl::not_null<ChordsModel *> chords_model_pointer_input,
    int first_child_number_input, int number_of_children_input,
    const StableIndex &stable_parent_index_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_child_number(first_child_number_input),
      number_of_children(number_of_children_input),
      stable_parent_index(stable_parent_index_input) {}

void InsertEmptyChange::redo() {
  chords_model_pointer->insert_empty_children_directly(
      first_child_number, number_of_children, stable_parent_index);
}

void InsertEmptyChange::undo() {
  chords_model_pointer->remove_rows_directly(
      first_child_number, number_of_children, stable_parent_index);
}
