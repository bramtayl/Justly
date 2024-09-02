#pragma once

#include <QUndoStack>
#include <cstddef>
#include <vector>

#include "justly/NoteChordColumn.hpp"
#include "note_chord/Note.hpp"

struct ChordsModel;

struct SetNotesCells : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const size_t chord_number;
  const size_t first_note_number;
  const NoteChordColumn left_column;
  const NoteChordColumn right_column;
  const std::vector<Note> old_notes;
  const std::vector<Note> new_notes;
  explicit SetNotesCells(ChordsModel *chords_model_pointer_input,
                         size_t chord_number_input,
                         size_t first_note_number_input,
                         NoteChordColumn left_column_input,
                         NoteChordColumn right_column_input,
                         const std::vector<Note> &old_notes_input,
                         const std::vector<Note> &new_notes_input,
                         QUndoCommand *parent_pointer_input = nullptr);
  void undo() override;
  void redo() override;
};
