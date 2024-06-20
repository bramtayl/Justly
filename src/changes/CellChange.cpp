#include "changes/CellChange.hpp"

#include <qvariant.h>  // for QVariant

#include <utility>  // for move

#include "models/ChordsModel.hpp"  // for ChordsModel

// set_cell will error if invalid, so need to check before
CellChange::CellChange(ChordsModel* chords_model_pointer_input,
                       const SongIndex &song_index_input,
                       QVariant old_value_input, QVariant new_value_input,
                       QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      song_index(song_index_input),
      old_value(std::move(old_value_input)),
      new_value(std::move(new_value_input)) {}

void CellChange::undo() {
  chords_model_pointer->set_cell(song_index, old_value);
}

void CellChange::redo() {
  chords_model_pointer->set_cell(song_index, new_value);
}
