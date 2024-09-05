#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "rational/Rational.hpp"

struct ChordsModel;

struct SetChordVelocityRatio : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const qsizetype chord_number;
  const Rational old_velocity_ratio;
  const Rational new_velocity_ratio;

  explicit SetChordVelocityRatio(ChordsModel *chords_model_pointer_input,
                                 qsizetype chord_number_input,
                                 const Rational &old_velocity_ratio_input,
                                 const Rational &new_velocity_ratio_input,
                                 QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
