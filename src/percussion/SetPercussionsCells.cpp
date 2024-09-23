#include "percussion/SetPercussionsCells.hpp"

#include <QtGlobal>

#include "justly/PercussionColumn.hpp"

#include "percussion/Percussion.hpp"
#include "percussion/PercussionsModel.hpp"
#include "rational/Rational.hpp"

static void replace_percussion_cells(
    PercussionsModel& percussions_model, qsizetype first_percussion_number,
    PercussionColumn left_column, PercussionColumn right_column,
    const QList<Percussion> &new_percussions) {
  auto *percussions_pointer = percussions_model.percussions_pointer;
  Q_ASSERT(percussions_pointer != nullptr);
  auto number_of_percussions = new_percussions.size();
  for (qsizetype replace_number = 0; replace_number < number_of_percussions;
       replace_number = replace_number + 1) {
    auto &percussion =
        (*percussions_pointer)[first_percussion_number + replace_number];
    const auto &new_percussion =
        new_percussions.at(replace_number);
    for (auto percussion_column = left_column;
         percussion_column <= right_column;
         percussion_column =
             static_cast<PercussionColumn>(percussion_column + 1)) {
      switch (percussion_column) {
      case percussion_instrument_column:
        percussion.percussion_instrument_pointer =
            new_percussion.percussion_instrument_pointer;
        break;
      case percussion_set_column:
        percussion.percussion_set_pointer =
            new_percussion.percussion_set_pointer;
        break;
      case percussion_beats_column:
        percussion.beats = new_percussion.beats;
        break;
      case percussion_velocity_ratio_column:
        percussion.velocity_ratio = new_percussion.velocity_ratio;
        break;
      case percussion_tempo_ratio_column:
        percussion.tempo_ratio = new_percussion.tempo_ratio;
        break;
      default:
        Q_ASSERT(false);
        break;
      }
    }
  }
  percussions_model.edited_cells(
      first_percussion_number, number_of_percussions, left_column,
      right_column);
}

SetPercussionsCells::SetPercussionsCells(
    PercussionsModel *percussions_model_pointer_input,
    qsizetype first_percussion_number_input, PercussionColumn left_column_input,
    PercussionColumn right_column_input,
    const QList<Percussion> &old_percussions_input,
    const QList<Percussion> &new_percussions_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      percussions_model_pointer(percussions_model_pointer_input),
      first_percussion_number(first_percussion_number_input),
      left_column(left_column_input), right_column(right_column_input),
      old_percussions(old_percussions_input),
      new_percussions(new_percussions_input) {
  Q_ASSERT(percussions_model_pointer != nullptr);
}

void SetPercussionsCells::undo() {
  replace_percussion_cells(*percussions_model_pointer, first_percussion_number,
                           left_column, right_column, old_percussions);
}

void SetPercussionsCells::redo() {
  replace_percussion_cells(*percussions_model_pointer, first_percussion_number,
                           left_column, right_column, new_percussions);
}
