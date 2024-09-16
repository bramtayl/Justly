#include "percussion/SetPercussionTempoRatio.hpp"

#include <QtGlobal>
#include <QList>

#include "justly/PercussionColumn.hpp"
#include "percussion/Percussion.hpp"
#include "percussion/PercussionsModel.hpp"

static void
set_percussion_tempo_ratio(PercussionsModel& percussions_model,
                           qsizetype percussion_number,
                           const Rational &new_tempo_ratio) {
  auto *percussions_pointer = percussions_model.percussions_pointer;
  Q_ASSERT(percussions_pointer != nullptr);
  (*percussions_pointer)[percussion_number]
      .tempo_ratio = new_tempo_ratio;
  percussions_model.edited_cells(
      percussion_number, 1, percussion_tempo_ratio_column,
      percussion_tempo_ratio_column);
}

SetPercussionTempoRatio::SetPercussionTempoRatio(
    PercussionsModel *percussions_model_pointer_input,
    qsizetype percussion_number_input, const Rational &old_tempo_input,
    const Rational &new_tempo_input, QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      percussions_model_pointer(percussions_model_pointer_input),
      percussion_number(percussion_number_input), old_tempo(old_tempo_input),
      new_tempo(new_tempo_input) {
  Q_ASSERT(percussions_model_pointer != nullptr);
}

void SetPercussionTempoRatio::undo() {
  set_percussion_tempo_ratio(*percussions_model_pointer, percussion_number,
                             old_tempo);
}

void SetPercussionTempoRatio::redo() {
  set_percussion_tempo_ratio(*percussions_model_pointer, percussion_number,
                             new_tempo);
}
