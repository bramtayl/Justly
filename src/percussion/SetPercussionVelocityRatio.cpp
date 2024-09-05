#include "percussion/SetPercussionVelocityRatio.hpp"

#include <QtGlobal>
#include <QList>
#include "justly/PercussionColumn.hpp"
#include "percussion/Percussion.hpp"
#include "percussion/PercussionsModel.hpp"

static void
set_percussion_velocity_ratio(PercussionsModel *percussions_model_pointer,
                              qsizetype percussion_number,
                              const Rational &new_velocity_ratio) {
  Q_ASSERT(percussions_model_pointer != nullptr);
  percussions_model_pointer->percussions[percussion_number]
      .velocity_ratio = new_velocity_ratio;
  percussions_model_pointer->edited_percussions_cells(
      percussion_number, 1, percussion_velocity_ratio_column,
      percussion_velocity_ratio_column);
}

SetPercussionVelocityRatio::SetPercussionVelocityRatio(
    PercussionsModel *percussions_model_pointer_input,
    qsizetype percussion_number_input, const Rational &old_velocity_ratio_input,
    const Rational &new_velocity_ratio_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      percussions_model_pointer(percussions_model_pointer_input),
      percussion_number(percussion_number_input),
      old_velocity_ratio(old_velocity_ratio_input),
      new_velocity_ratio(new_velocity_ratio_input) {
  Q_ASSERT(percussions_model_pointer != nullptr);
}

void SetPercussionVelocityRatio::undo() {
  set_percussion_velocity_ratio(percussions_model_pointer, percussion_number,
                                old_velocity_ratio);
}

void SetPercussionVelocityRatio::redo() {
  set_percussion_velocity_ratio(percussions_model_pointer, percussion_number,
                                new_velocity_ratio);
}
