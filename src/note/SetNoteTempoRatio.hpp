#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "rational/Rational.hpp"

struct NotesModel;

struct SetNoteTempoRatio : public QUndoCommand {
  NotesModel *const notes_model_pointer;
  const qsizetype note_number;
  const Rational old_tempo;
  const Rational new_tempo;

  explicit SetNoteTempoRatio(NotesModel *notes_model_pointer_input,
                             qsizetype note_number_input,
                             const Rational &old_tempo_input,
                             const Rational &new_tempo_input,
                             QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
