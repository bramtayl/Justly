#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "rational/Rational.hpp"

struct PercussionsModel;

struct SetPercussionTempoRatio : public QUndoCommand {
  PercussionsModel *const percussions_model_pointer;
  const qsizetype percussion_number;
  const Rational old_tempo;
  const Rational new_tempo;

  explicit SetPercussionTempoRatio(
      PercussionsModel *percussions_model_pointer_input,
      qsizetype percussion_number_input, const Rational &old_tempo_input,
      const Rational &new_tempo_input,
      QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
