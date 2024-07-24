#include "changes/CellChange.hpp"

#include <QVariant>
#include <QtGlobal>
#include <utility>

#include "justly/ChordsModel.hpp"

CellChange::CellChange(ChordsModel *chords_model_pointer_input,
                       const CellIndex &cell_index_input,
                       QVariant old_value_input, QVariant new_value_input,
                       QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      cell_index(cell_index_input), old_value(std::move(old_value_input)),
      new_value(std::move(new_value_input)) {}

void CellChange::undo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->set_cell(cell_index, old_value);
}

void CellChange::redo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->set_cell(cell_index, new_value);
}
