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
  editor_pointer->get_chords_model().directly_set_data(stable_index, new_value);
}

void CellChange::undo() {
  editor_pointer->get_chords_model().directly_set_data(stable_index,
                                 old_value);
}
