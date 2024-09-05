#pragma once

#include <QUndoStack>
#include <cstddef>

#include "rational/Rational.hpp"

struct PercussionsModel;

struct SetPercussionBeats : public QUndoCommand {
  PercussionsModel *const percussions_model_pointer;
  const size_t percussion_number;
  const Rational old_beats;
  const Rational new_beats;

  explicit SetPercussionBeats(PercussionsModel *percussions_model_pointer_input,
                              size_t percussion_number_input,
                              const Rational &old_beats_input,
                              const Rational &new_beats_input,
                              QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
