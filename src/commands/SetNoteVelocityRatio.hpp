#pragma once

#include <QUndoStack>
#include <cstddef>

#include "rational/Rational.hpp"

struct ChordsModel;

class SetNoteVelocityRatio : public QUndoCommand {
private:
  ChordsModel *const chords_model_pointer;
  const size_t chord_number;
  const size_t note_number;
  const Rational old_velocity_ratio;
  const Rational new_velocity_ratio;

public:
  explicit SetNoteVelocityRatio(ChordsModel *chords_model_pointer_input,
                       size_t chord_number_input, size_t note_number_input,
                       const Rational& old_velocity_ratio_input, const Rational& new_velocity_ratio_input, 
                       QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
