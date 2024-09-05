#pragma once

#include <QUndoStack>
#include <cstddef>
#include <vector>

#include "percussion/Percussion.hpp"

struct PercussionsModel;

struct InsertPercussions : public QUndoCommand {
  PercussionsModel *const percussions_model_pointer;
  const size_t first_percussion_number;
  const std::vector<Percussion> new_percussions;

  InsertPercussions(PercussionsModel *percussions_model_pointer_input,
                    size_t first_percussion_number_input,
                    const std::vector<Percussion> &new_percussions_input,
                    QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
