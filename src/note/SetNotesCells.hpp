#pragma once

#include <QUndoStack>
#include <cstddef>
#include <vector>

#include "justly/NoteColumn.hpp"
#include "note/Note.hpp"

struct NotesModel;

struct SetNotesCells : public QUndoCommand {
  NotesModel *const notes_model_pointer;
  const size_t first_note_number;
  const NoteColumn left_column;
  const NoteColumn right_column;
  const std::vector<Note> old_notes;
  const std::vector<Note> new_notes;
  explicit SetNotesCells(NotesModel *notes_model_pointer_input,
                         size_t first_note_number_input,
                         NoteColumn left_column_input,
                         NoteColumn right_column_input,
                         const std::vector<Note> &old_notes_input,
                         const std::vector<Note> &new_notes_input,
                         QUndoCommand *parent_pointer_input = nullptr);
  void undo() override;
  void redo() override;
};
