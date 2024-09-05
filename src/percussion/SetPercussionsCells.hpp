#pragma once

#include <QUndoStack>
#include <QtGlobal>
#include <QList>

#include "justly/PercussionColumn.hpp"
#include "percussion/Percussion.hpp" // IWYU pragma: keep

struct PercussionsModel;

struct SetPercussionsCells : public QUndoCommand {
  PercussionsModel *const percussions_model_pointer;
  const qsizetype first_percussion_number;
  const PercussionColumn left_column;
  const PercussionColumn right_column;
  const QList<Percussion> old_percussions;
  const QList<Percussion> new_percussions;
  explicit SetPercussionsCells(
      PercussionsModel *percussions_model_pointer_input,
      qsizetype first_percussion_number_input, PercussionColumn left_column_input,
      PercussionColumn right_column_input,
      const QList<Percussion> &old_percussions_input,
      const QList<Percussion> &new_percussions_input,
      QUndoCommand *parent_pointer_input = nullptr);
  void undo() override;
  void redo() override;
};
