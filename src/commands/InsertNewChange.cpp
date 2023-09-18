#include "commands/InsertNewChange.h"

#include <qpointer.h>    // for QPointer

#include "models/ChordsModel.h"  // for ChordsModel
#include "Editor.h"       // for Editor

class QModelIndex;

InsertNewChange::InsertNewChange(Editor &editor_input, int first_index_input,
                                 int number_of_rows_input,
                                 const QModelIndex &parent_index_input,
                                 QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      editor(editor_input),
      first_index(first_index_input),
      number_of_children(number_of_rows_input),
      stable_parent_index(
          editor.chords_model_pointer->get_stable_index(parent_index_input)) {}

void InsertNewChange::redo() {
  editor.register_changed();
  editor.chords_model_pointer->insertRows(
      first_index, number_of_children,
      editor.chords_model_pointer->get_unstable_index(stable_parent_index));
}

void InsertNewChange::undo() {
  editor.chords_model_pointer->removeRows(
      first_index, number_of_children,
      editor.chords_model_pointer->get_unstable_index(stable_parent_index));
}
