#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "rational/Rational.hpp"

struct NotesModel;

struct SetNoteVelocityRatio : public QUndoCommand {
  NotesModel *const notes_model_pointer;
  const qsizetype note_number;
  const Rational old_velocity_ratio;
  const Rational new_velocity_ratio;

  explicit SetNoteVelocityRatio(NotesModel *notes_model_pointer_input,
                                qsizetype note_number_input,
                                const Rational &old_velocity_ratio_input,
                                const Rational &new_velocity_ratio_input,
                                QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
