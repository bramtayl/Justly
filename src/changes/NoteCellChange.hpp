#pragma once

#include <QUndoStack>
#include <QVariant>
#include <cstddef>

#include "justly/NoteChordField.hpp"

class ChordsModel;

class NoteCellChange : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const size_t chord_number;
  const size_t note_number;
  const NoteChordField note_chord_field;
  const QVariant old_value;
  QVariant new_value;

public:
  explicit NoteCellChange(ChordsModel *chords_model_pointer_input,
                          size_t chord_number_input, size_t note_number_input,
                          NoteChordField note_chord_field_input,
                          QVariant old_value_input, QVariant new_value_input,
                          QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
