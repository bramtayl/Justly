#include "percussion/SetPercussionBeats.hpp"

#include <QList>
#include <QtGlobal>

#include "justly/PercussionColumn.hpp"
#include "percussion/Percussion.hpp"
#include "percussion/PercussionsModel.hpp"

static void set_percussion_beats(PercussionsModel& percussions_model,
                                 qsizetype percussion_number,
                                 const Rational &new_beats) {
  auto *percussions_pointer = percussions_model.percussions_pointer;
  Q_ASSERT(percussions_pointer != nullptr);
  (*percussions_pointer)[percussion_number].beats =
      new_beats;
  percussions_model.edited_cells(
      percussion_number, 1, percussion_beats_column, percussion_beats_column);
}

SetPercussionBeats::SetPercussionBeats(
    PercussionsModel *percussions_model_pointer_input,
    qsizetype percussion_number_input, const Rational &old_beats_input,
    const Rational &new_beats_input, QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      percussions_model_pointer(percussions_model_pointer_input),
      percussion_number(percussion_number_input), old_beats(old_beats_input),
      new_beats(new_beats_input) {
  Q_ASSERT(percussions_model_pointer != nullptr);
}

void SetPercussionBeats::undo() {
  set_percussion_beats(*percussions_model_pointer, percussion_number, old_beats);
}

void SetPercussionBeats::redo() {
  set_percussion_beats(*percussions_model_pointer, percussion_number, new_beats);
}
