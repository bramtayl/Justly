#pragma once

#include <QUndoStack>
#include <cstddef>
#include <vector>

#include "justly/ChordsModel.hpp"
#include "justly/NoteChord.hpp"
#include "justly/NoteChordField.hpp"

class ChordCellChanges : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const size_t chord_number;
  const NoteChordField left_note_chord_field;
  const NoteChordField right_note_chord_field;
  const std::vector<NoteChord> old_note_chord_templates;
  const std::vector<NoteChord> new_note_chord_templates;

public:
  explicit ChordCellChanges(
      ChordsModel *chords_model_pointer_input, size_t chord_number_input,
      NoteChordField left_note_chord_field_input,
      NoteChordField right_note_chord_field_input,
      const std::vector<NoteChord> &old_note_chord_templates_input,
      const std::vector<NoteChord> &new_note_chord_templates_input,
      QUndoCommand *parent_pointer_input = nullptr);
  void undo() override;
  void redo() override;
};
