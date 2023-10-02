#include "commands/CellChange.h"

#include <qvariant.h>  // for QVariant

#include <utility>  // for move

#include "main/Editor.h"         // for Editor
#include "models/ChordsModel.h"  // for ChordsModel

class QModelIndex;

// directly_set_data will error if invalid, so need to check before
CellChange::CellChange(gsl::not_null<Editor *> editor_pointer_input,
                       const QModelIndex &index_input, QVariant old_value_input,
                       QVariant new_value_input,
                       QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      editor_pointer(editor_pointer_input),
      stable_index(editor_pointer_input->get_chords_model().get_stable_index(
          index_input)),
      old_value(std::move(old_value_input)),
      new_value(std::move(new_value_input)) {}

void CellChange::redo() {
  editor_pointer->register_changed();
  auto &chords_model = editor_pointer->get_chords_model();
  if (!first_time) {
    chords_model.directly_set_data(
        chords_model.get_unstable_index(stable_index), new_value);
  }
  first_time = false;
}

void CellChange::undo() {
  auto &chords_model = editor_pointer->get_chords_model();
  chords_model.directly_set_data(chords_model.get_unstable_index(stable_index),
                                 old_value);
}
