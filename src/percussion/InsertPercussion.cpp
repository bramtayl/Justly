#include "percussion/InsertPercussion.hpp"

#include <QtGlobal>
#include <QList>


#include "percussion/PercussionsModel.hpp"

InsertPercussion::InsertPercussion(
    PercussionsModel *percussions_model_pointer_input,
    qsizetype percussion_number_input, const Percussion &new_percussion_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      percussions_model_pointer(percussions_model_pointer_input),
      percussion_number(percussion_number_input),
      new_percussion(new_percussion_input) {
  Q_ASSERT(percussions_model_pointer != nullptr);
}

auto InsertPercussion::undo() -> void {
  remove_percussions(percussions_model_pointer, percussion_number, 1);
}

auto InsertPercussion::redo() -> void {
  Q_ASSERT(percussions_model_pointer != nullptr);
  auto &percussions = percussions_model_pointer->percussions;

  percussions_model_pointer->begin_insert_rows(percussion_number, 1);
  percussions.insert(percussions.begin() + static_cast<int>(percussion_number),
                     new_percussion);
  percussions_model_pointer->end_insert_rows();
}
