#pragma once

#include <QUndoStack>
#include <cstddef>
#include <vector>

#include "justly/NoteChord.hpp"
#include "justly/NoteChordField.hpp"

class ChordsModel;

class ChordCellChanges : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const size_t chord_number;
  const NoteChordField left_field;
  const NoteChordField right_field;
  const std::vector<NoteChord> old_note_chords;
  const std::vector<NoteChord> new_note_chords;

public:
  explicit ChordCellChanges(ChordsModel *chords_model_pointer_input,
                            size_t chord_number_input,
                            NoteChordField left_field_input,
                            NoteChordField right_field_input,
                            const std::vector<NoteChord> &old_note_chords_input,
                            const std::vector<NoteChord> &new_note_chords_input,
                            QUndoCommand *parent_pointer_input = nullptr);
  void undo() override;
  void redo() override;
};
