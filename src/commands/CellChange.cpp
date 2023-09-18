#include "commands/CellChange.h"

#include <qnamespace.h>  // for DisplayRole
#include <qpointer.h>    // for QPointer
#include <qvariant.h>    // for QVariant

#include <utility>  // for move

#include "models/ChordsModel.h"  // for ChordsModel
#include "Editor.h"       // for Editor

class QModelIndex;

// directly_set_data will error if invalid, so need to check before
CellChange::CellChange(Editor &editor_input, const QModelIndex &parent_index_input,
                 QVariant new_value_input, QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      editor(editor_input),
      stable_parent_index(
          editor.chords_model_pointer->get_stable_index(parent_index_input)),
      old_value(editor.chords_model_pointer->data(parent_index_input,
                                                  Qt::DisplayRole)),
      new_value(std::move(new_value_input)) {}

void CellChange::redo() {
  editor.register_changed();
  editor.chords_model_pointer->directly_set_data(
      editor.chords_model_pointer->get_unstable_index(stable_parent_index),
      new_value);
}

void CellChange::undo() {
  editor.chords_model_pointer->directly_set_data(
      editor.chords_model_pointer->get_unstable_index(stable_parent_index),
      old_value);
}
