#include "changes/CellChange.hpp"

#include <qvariant.h>  // for QVariant

#include <utility>  // for move

#include "changes/ChangeId.hpp"
#include "models/ChordsModel.hpp"  // for ChordsModel

CellChange::CellChange(ChordsModel *chords_model_pointer_input,
                       const SongIndex &song_index_input,
                       QVariant old_value_input, QVariant new_value_input,
                       QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      song_index(song_index_input),
      old_value(std::move(old_value_input)),
      new_value(std::move(new_value_input)) {}

auto CellChange::id() const -> int { return starting_key_id; }

auto CellChange::mergeWith(const QUndoCommand *next_command_pointer) -> bool {
  const auto *next_command_pointer_fixed =
      dynamic_cast<const CellChange *>(next_command_pointer);
  if (next_command_pointer_fixed->song_index == song_index) {
    new_value = next_command_pointer_fixed->new_value;
    return true;
  }
  return false;
}

void CellChange::undo() {
  chords_model_pointer->set_cell(song_index, old_value);
}

void CellChange::redo() {
  chords_model_pointer->set_cell(song_index, new_value);
}
