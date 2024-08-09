#pragma once

#include <QUndoStack>
#include <QVariant>
#include <cstddef>

#include "justly/NoteChordColumn.hpp"

struct ChordsModel;

class SetChordCell : public QUndoCommand {
private:
  ChordsModel *const chords_model_pointer;
  const size_t chord_number;
  const NoteChordColumn note_chord_column;
  const QVariant old_value;
  const QVariant new_value;

public:
  explicit SetChordCell(ChordsModel *chords_model_pointer_input,
                        size_t chord_number_input,
                        NoteChordColumn note_chord_column_input,
                        QVariant old_value_input, QVariant new_value_input,
                        QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
