#pragma once

#include <QList>
#include <QUndoStack>
#include <QtGlobal>

#include "justly/NoteColumn.hpp"

struct NotesModel;
struct Note;

struct SetNotesCells : public QUndoCommand {
  NotesModel *const notes_model_pointer;
  const qsizetype first_note_number;
  const NoteColumn left_column;
  const NoteColumn right_column;
  const QList<Note> old_notes;
  const QList<Note> new_notes;
  explicit SetNotesCells(NotesModel *notes_model_pointer_input,
                         qsizetype first_note_number_input,
                         NoteColumn left_column_input,
                         NoteColumn right_column_input,
                         const QList<Note> &old_notes_input,
                         const QList<Note> &new_notes_input,
                         QUndoCommand *parent_pointer_input = nullptr);
  void undo() override;
  void redo() override;
};
