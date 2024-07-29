#pragma once

#include <QUndoStack>
#include <QVariant>

#include "justly/CellIndex.hpp"

class ChordsModel;

class CellChange : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const CellIndex cell_index;
  const QVariant old_value;
  const QVariant new_value;

public:
  explicit CellChange(ChordsModel *chords_model_pointer_input,
                           const CellIndex& cell_index_input,
                           QVariant old_value_input, QVariant new_value_input,
                           QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
