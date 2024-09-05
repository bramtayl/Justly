#pragma once

#include <QUndoStack>
#include <cstddef>

#include "rational/Rational.hpp"

struct PercussionsModel;

struct SetPercussionVelocityRatio : public QUndoCommand {
  PercussionsModel *const percussions_model_pointer;
  const size_t percussion_number;
  const Rational old_velocity_ratio;
  const Rational new_velocity_ratio;

  explicit SetPercussionVelocityRatio(
      PercussionsModel *percussions_model_pointer_input,
      size_t percussion_number_input, const Rational &old_velocity_ratio_input,
      const Rational &new_velocity_ratio_input,
      QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
