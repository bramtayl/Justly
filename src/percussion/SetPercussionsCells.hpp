#pragma once

#include <QUndoStack>
#include <cstddef>
#include <vector>

#include "justly/PercussionColumn.hpp"
#include "percussion/Percussion.hpp"

struct PercussionsModel;

struct SetPercussionsCells : public QUndoCommand {
  PercussionsModel *const percussions_model_pointer;
  const size_t first_percussion_number;
  const PercussionColumn left_column;
  const PercussionColumn right_column;
  const std::vector<Percussion> old_percussions;
  const std::vector<Percussion> new_percussions;
  explicit SetPercussionsCells(
      PercussionsModel *percussions_model_pointer_input,
      size_t first_percussion_number_input, PercussionColumn left_column_input,
      PercussionColumn right_column_input,
      const std::vector<Percussion> &old_percussions_input,
      const std::vector<Percussion> &new_percussions_input,
      QUndoCommand *parent_pointer_input = nullptr);
  void undo() override;
  void redo() override;
};
