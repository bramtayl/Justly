#pragma once

#include <QUndoStack>
#include <QtGlobal>
#include <QList>

#include "note/Note.hpp" // IWYU pragma: keep

struct NotesModel;

struct InsertNotes : public QUndoCommand {
  NotesModel *const notes_model_pointer;
  const qsizetype first_note_number;
  const QList<Note> new_notes;

  InsertNotes(NotesModel *notes_model_pointer_input,
              qsizetype first_note_number_input,
              const QList<Note> &new_notes_input,
              QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
