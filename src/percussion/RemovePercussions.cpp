#include "percussion/RemovePercussions.hpp"

#include <QtGlobal>

#include "percussion/Percussion.hpp" // IWYU pragma: keep
#include "percussion/PercussionsModel.hpp"

RemovePercussions::RemovePercussions(
    PercussionsModel *percussions_model_pointer_input,
    qsizetype first_percussion_number_input,
    const QList<Percussion> &old_percussions_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      percussions_model_pointer(percussions_model_pointer_input),
      first_percussion_number(first_percussion_number_input),
      old_percussions(old_percussions_input) {
  Q_ASSERT(percussions_model_pointer != nullptr);
}

auto RemovePercussions::undo() -> void {
  insert_percussions(*percussions_model_pointer, first_percussion_number,
                     old_percussions);
}

auto RemovePercussions::redo() -> void {
  remove_percussions(*percussions_model_pointer, first_percussion_number,
                     old_percussions.size());
}
