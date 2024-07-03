#include "changes/CellChange.hpp"

#include <qassert.h>   // for Q_ASSERT
#include <qvariant.h>  // for QVariant

#include <utility>  // for move

#include "justly/ChangeId.hpp"    // for starting_key_id
#include "justly/ChordsModel.hpp"  // for ChordsModel

CellChange::CellChange(ChordsModel *chords_model_pointer_input,
                       const CellIndex &song_index_input,
                       QVariant old_value_input, QVariant new_value_input,
                       QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      cell_index(song_index_input),
      old_value(std::move(old_value_input)),
      new_value(std::move(new_value_input)) {}

auto CellChange::id() const -> int { return cell_id; }

auto CellChange::mergeWith(const QUndoCommand *next_command_pointer) -> bool {
  Q_ASSERT(next_command_pointer != nullptr);

  const auto *next_cell_change_pointer =
      dynamic_cast<const CellChange *>(next_command_pointer);

  Q_ASSERT(next_cell_change_pointer != nullptr);
  if (next_cell_change_pointer->cell_index == cell_index) {
    new_value = next_cell_change_pointer->new_value;
    return true;
  }
  return false;
}

void CellChange::undo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->set_cell_directly(cell_index, old_value);
}

void CellChange::redo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->set_cell_directly(cell_index, new_value);
}
