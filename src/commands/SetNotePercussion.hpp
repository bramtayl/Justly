#pragma once

#include <QUndoStack>
#include <cstddef>

struct ChordsModel;
struct Percussion;

struct SetNotePercussion : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const size_t chord_number;
  const size_t note_number;
  const Percussion *const old_percussion_pointer;
  const Percussion *const new_percussion_pointer;

  explicit SetNotePercussion(ChordsModel *chords_model_pointer_input,
                             size_t chord_number_input,
                             size_t note_number_input,
                             const Percussion *old_percussion_pointer_input,
                             const Percussion *new_percussion_pointer_input,
                             QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
