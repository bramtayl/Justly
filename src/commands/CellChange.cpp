#include "commands/CellChange.h"

#include <qvariant.h>  // for QVariant

#include <utility>  // for move

#include "main/Editor.h"         // for Editor
#include "models/ChordsModel.h"  // for ChordsModel

class QModelIndex;

// directly_set_data will error if invalid, so need to check before
CellChange::CellChange(gsl::not_null<Editor*> editor_pointer_input, const QModelIndex &index_input,
                       QVariant old_value_input, QVariant new_value_input,
                       QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      editor_pointer(editor_pointer_input),
      stable_index(editor_pointer_input->chords_model_pointer->get_stable_index(index_input)),
      old_value(std::move(old_value_input)),
      new_value(std::move(new_value_input)) {}

void CellChange::redo() {
  editor_pointer->register_changed();
  if (!first_time) {
    editor_pointer->chords_model_pointer->directly_set_data(
        editor_pointer->chords_model_pointer->get_unstable_index(stable_index),
        new_value);
  }
  first_time = false;
}

void CellChange::undo() {
  editor_pointer->chords_model_pointer->directly_set_data(
      editor_pointer->chords_model_pointer->get_unstable_index(stable_index), old_value);
}
