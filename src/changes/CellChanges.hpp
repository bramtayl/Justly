#pragma once

#include <QUndoStack>
#include <vector>

#include "justly/CellIndex.hpp"
#include "justly/NoteChord.hpp"
#include "justly/NoteChordField.hpp"

class ChordsModel;

class CellChanges : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const CellIndex top_left_cell_index;
  const NoteChordField right_field;
  const std::vector<NoteChord> old_note_chords;
  const std::vector<NoteChord> new_note_chords;

public:
  explicit CellChanges(ChordsModel *chords_model_pointer_input,
                       const CellIndex &top_left_cell_index_input,
                       NoteChordField right_field_input,
                       const std::vector<NoteChord> &old_note_chords_input,
                       const std::vector<NoteChord> &new_note_chords_input,
                       QUndoCommand *parent_pointer_input = nullptr);
  void undo() override;
  void redo() override;
};
