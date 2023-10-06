#include "commands/RemoveChange.h"

#include "main/TreeNode.h"       // for TreeNode
#include "models/ChordsModel.h"  // for ChordsModel

class QModelIndex;

RemoveChange::RemoveChange(gsl::not_null<ChordsModel *> chords_model_pointer_input,
                           int first_index_input, int number_of_rows_input,
                           const QModelIndex &parent_index_input,
                           QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_index(first_index_input),
      number_of_children(number_of_rows_input),
      stable_parent_index(chords_model_pointer->get_stable_index(
          parent_index_input)) {}

// remove_save will check for errors, so no need to check here
auto RemoveChange::redo() -> void {
  chords_model_pointer->remove_save(first_index, number_of_children,
                           chords_model_pointer->get_unstable_index(stable_parent_index),
                           &deleted_children);
}

auto RemoveChange::undo() -> void {
  chords_model_pointer->insert_children(
      first_index, &deleted_children,
      chords_model_pointer->get_unstable_index(stable_parent_index));
}
