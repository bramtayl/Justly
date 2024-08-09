#pragma once

#include <QUndoStack>
#include <vector>

#include "indices/RowRange.hpp"
#include "justly/NoteChordColumn.hpp"
#include "note_chord/NoteChord.hpp"

struct ChordsModel;

class SetCells : public QUndoCommand {
private:
  ChordsModel *const chords_model_pointer;
  const std::vector<RowRange> row_ranges;
  const std::vector<NoteChord> old_note_chords;
  const std::vector<NoteChord> new_note_chords;
  NoteChordColumn left_column;
  NoteChordColumn right_column;

public:
  explicit SetCells(ChordsModel *chords_model_pointer_input,
                    const std::vector<RowRange> &row_ranges_input,
                    const std::vector<NoteChord> &old_note_chords_input,
                    const std::vector<NoteChord> &new_note_chords_input,
                    NoteChordColumn left_column_input,
                    NoteChordColumn right_column_input,
                    QUndoCommand *parent_pointer_input = nullptr);
  void undo() override;
  void redo() override;
};
