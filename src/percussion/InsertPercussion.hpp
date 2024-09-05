#pragma once

#include <QUndoStack>
#include <cstddef>

#include "percussion/Percussion.hpp"

struct PercussionsModel;

struct InsertPercussion : public QUndoCommand {
  PercussionsModel *const percussions_model_pointer;
  const size_t percussion_number;
  const Percussion new_percussion;

  InsertPercussion(PercussionsModel *percussions_model_pointer_input,
                   size_t percussion_number_input,
                   const Percussion &new_percussion_input,
                   QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
