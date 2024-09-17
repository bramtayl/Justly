#include "percussion/InsertPercussions.hpp"

#include <QtGlobal>

#include "percussion/PercussionsModel.hpp"

InsertPercussions::InsertPercussions(
    PercussionsModel *percussions_model_pointer_input,
    qsizetype first_percussion_number_input,
    const QList<Percussion> &new_percussions_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      percussions_model_pointer(percussions_model_pointer_input),
      first_percussion_number(first_percussion_number_input),
      new_percussions(new_percussions_input) {
  Q_ASSERT(percussions_model_pointer != nullptr);
}

auto InsertPercussions::undo() -> void {
  remove_percussions(*percussions_model_pointer, first_percussion_number,
                     new_percussions.size());
}

auto InsertPercussions::redo() -> void {
  insert_percussions(*percussions_model_pointer, first_percussion_number,
                     new_percussions);
}
