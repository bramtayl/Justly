#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "note/Note.hpp"

struct NotesModel;

struct InsertNote : public QUndoCommand {
  NotesModel *const notes_model_pointer;
  const qsizetype note_number;
  const Note new_note;

  InsertNote(NotesModel *notes_model_pointer_input, qsizetype note_number_input,
             const Note &new_note_input,
             QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
