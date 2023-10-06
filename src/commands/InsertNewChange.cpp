#include "commands/InsertNewChange.h"

#include "models/ChordsModel.h"  // for ChordsModel

class QModelIndex;

InsertNewChange::InsertNewChange(gsl::not_null<ChordsModel *> chords_model_pointer_input,
                                 int first_index_input,
                                 int number_of_rows_input,
                                 const QModelIndex &parent_index_input,
                                 QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_index(first_index_input),
      number_of_children(number_of_rows_input),
      stable_parent_index(chords_model_pointer->get_stable_index(
          parent_index_input)) {}

void InsertNewChange::redo() {
  chords_model_pointer->insertRows(first_index, number_of_children,
                          chords_model_pointer->get_unstable_index(stable_parent_index));
}

void InsertNewChange::undo() {
  chords_model_pointer->removeRows(first_index, number_of_children,
                          chords_model_pointer->get_unstable_index(stable_parent_index));
}
