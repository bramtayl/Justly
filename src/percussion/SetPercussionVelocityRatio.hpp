#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "rational/Rational.hpp"

struct PercussionsModel;

struct SetPercussionVelocityRatio : public QUndoCommand {
  PercussionsModel *const percussions_model_pointer;
  const qsizetype percussion_number;
  const Rational old_velocity_ratio;
  const Rational new_velocity_ratio;

  explicit SetPercussionVelocityRatio(
      PercussionsModel *percussions_model_pointer_input,
      qsizetype percussion_number_input, const Rational &old_velocity_ratio_input,
      const Rational &new_velocity_ratio_input,
      QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
