#include "percussion/SetPercussionSet.hpp"

#include <QtGlobal>
#include <QList>

#include "justly/PercussionColumn.hpp"
#include "percussion/Percussion.hpp"
#include "percussion/PercussionsModel.hpp"

static void
set_percussion_instrument(PercussionsModel *percussions_model_pointer,
                          qsizetype percussion_number,
                          const PercussionSet *new_percussion_set_pointer) {
  Q_ASSERT(percussions_model_pointer != nullptr);
  auto *percussions_pointer = percussions_model_pointer->percussions_pointer;
  Q_ASSERT(percussions_pointer != nullptr);
  (*percussions_pointer)[percussion_number]
      .percussion_set_pointer = new_percussion_set_pointer;
  percussions_model_pointer->edited_percussions_cells(
      percussion_number, 1, percussion_set_column, percussion_set_column);
}

SetPercussionSet::SetPercussionSet(
    PercussionsModel *percussions_model_pointer_input,
    qsizetype percussion_number_input,
    const PercussionSet *old_percussion_set_pointer_input,
    const PercussionSet *new_percussion_set_pointer_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      percussions_model_pointer(percussions_model_pointer_input),
      percussion_number(percussion_number_input),
      old_percussion_set_pointer(old_percussion_set_pointer_input),
      new_percussion_set_pointer(new_percussion_set_pointer_input) {
  Q_ASSERT(percussions_model_pointer != nullptr);
}

void SetPercussionSet::undo() {
  set_percussion_instrument(percussions_model_pointer, percussion_number,
                            old_percussion_set_pointer);
}

void SetPercussionSet::redo() {
  set_percussion_instrument(percussions_model_pointer, percussion_number,
                            new_percussion_set_pointer);
}
