#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "percussion/Percussion.hpp"

struct PercussionsModel;

struct InsertPercussion : public QUndoCommand {
  PercussionsModel *const percussions_model_pointer;
  const qsizetype percussion_number;
  const Percussion new_percussion;

  InsertPercussion(PercussionsModel *percussions_model_pointer_input,
                   qsizetype percussion_number_input,
                   const Percussion &new_percussion_input,
                   QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
