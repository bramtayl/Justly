#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "rational/Rational.hpp"

struct NotesModel;

struct SetNoteBeats : public QUndoCommand {
  NotesModel *const notes_model_pointer;
  const qsizetype note_number;
  const Rational old_beats;
  const Rational new_beats;

  explicit SetNoteBeats(NotesModel *notes_model_pointer_input,
                        qsizetype note_number_input,
                        const Rational &old_beats_input,
                        const Rational &new_beats_input,
                        QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
