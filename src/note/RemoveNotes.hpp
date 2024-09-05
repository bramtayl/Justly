#pragma once

#include <QUndoStack>
#include <QtGlobal>
#include <QList>

#include "note/Note.hpp" // IWYU pragma: keep

struct NotesModel;

struct RemoveNotes : public QUndoCommand {
  NotesModel *const notes_model_pointer;
  const qsizetype first_note_number;
  const QList<Note> old_notes;

  RemoveNotes(NotesModel *notes_model_pointer_input,
              qsizetype first_note_number_input,
              const QList<Note> &old_notes_input,
              QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
