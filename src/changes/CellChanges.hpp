#pragma once

#include <QUndoStack>
#include <vector>

#include "justly/RowRange.hpp"
#include "justly/NoteChord.hpp"
#include "justly/NoteChordField.hpp"

class ChordsModel;

class CellChanges : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const std::vector<RowRange> row_ranges;
  NoteChordField left_field;
  NoteChordField right_field;
  const std::vector<NoteChord> old_note_chords;
  const std::vector<NoteChord> new_note_chords;

public:
  explicit CellChanges(
      ChordsModel *chords_model_pointer_input,
      const std::vector<RowRange> &row_replacement_ranges_input,
      NoteChordField left_field_input, NoteChordField right_field_input,
      const std::vector<NoteChord> &old_note_chords_input,
      const std::vector<NoteChord> &new_note_chords_input,
      QUndoCommand *parent_pointer_input = nullptr);
  void undo() override;
  void redo() override;
};
