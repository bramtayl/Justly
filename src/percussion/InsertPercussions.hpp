#pragma once

#include <QUndoStack>
#include <QtGlobal>
#include <QList>

#include "percussion/Percussion.hpp" // IWYU pragma: keep

struct PercussionsModel;

struct InsertPercussions : public QUndoCommand {
  PercussionsModel *const percussions_model_pointer;
  const qsizetype first_percussion_number;
  const QList<Percussion> new_percussions;

  InsertPercussions(PercussionsModel *percussions_model_pointer_input,
                    qsizetype first_percussion_number_input,
                    const QList<Percussion> &new_percussions_input,
                    QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
