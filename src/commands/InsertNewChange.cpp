#include "commands/InsertNewChange.h"

#include "main/Editor.h"         // for Editor
#include "models/ChordsModel.h"  // for ChordsModel

class QModelIndex;

InsertNewChange::InsertNewChange(gsl::not_null<Editor *> editor_pointer_input,
                                 int first_index_input,
                                 int number_of_rows_input,
                                 const QModelIndex &parent_index_input,
                                 QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      editor_pointer(editor_pointer_input),
      first_index(first_index_input),
      number_of_children(number_of_rows_input),
      stable_parent_index(
          editor_pointer->chords_model_pointer->get_stable_index(
              parent_index_input)) {}

void InsertNewChange::redo() {
  editor_pointer->register_changed();
  editor_pointer->chords_model_pointer->insertRows(
      first_index, number_of_children,
      editor_pointer->chords_model_pointer->get_unstable_index(
          stable_parent_index));
}

void InsertNewChange::undo() {
  editor_pointer->chords_model_pointer->removeRows(
      first_index, number_of_children,
      editor_pointer->chords_model_pointer->get_unstable_index(
          stable_parent_index));
}