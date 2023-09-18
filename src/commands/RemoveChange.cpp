#include "commands/RemoveChange.h"

#include <qpointer.h>    // for QPointer

#include "models/ChordsModel.h"  // for ChordsModel
#include "Editor.h"       // for Editor
#include "TreeNode.h"     // for TreeNode

class QModelIndex;

RemoveChange::RemoveChange(Editor &editor_input, int first_index_input,
               int number_of_rows_input, const QModelIndex &parent_index_input,
               QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      editor(editor_input),
      first_index(first_index_input),
      number_of_children(number_of_rows_input),
      stable_parent_index(
          editor.chords_model_pointer->get_stable_index(parent_index_input)){};

// remove_save will check for errors, so no need to check here
auto RemoveChange::redo() -> void {
  editor.register_changed();
  editor.chords_model_pointer->remove_save(
      first_index, number_of_children,
      editor.chords_model_pointer->get_unstable_index(stable_parent_index),
      deleted_children);
}

auto RemoveChange::undo() -> void {
  editor.chords_model_pointer->insert_children(
      first_index, deleted_children,
      editor.chords_model_pointer->get_unstable_index(stable_parent_index));
}
