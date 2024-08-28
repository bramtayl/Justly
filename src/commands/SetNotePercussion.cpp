#include "commands/SetNotePercussion.hpp"

#include <QtGlobal>

#include "other/ChordsModel.hpp"

struct Percussion;

SetNotePercussion::SetNotePercussion(
    ChordsModel *chords_model_pointer_input, size_t chord_number_input,
    size_t note_number_input, const Percussion *old_percussion_pointer_input,
    const Percussion *new_percussion_pointer_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), note_number(note_number_input),
      old_percussion_pointer(old_percussion_pointer_input),
      new_percussion_pointer(new_percussion_pointer_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetNotePercussion::undo() {
  chords_model_pointer->set_note_percussion(chord_number, note_number,
                                            old_percussion_pointer);
}

void SetNotePercussion::redo() {
  chords_model_pointer->set_note_percussion(chord_number, note_number,
                                            new_percussion_pointer);
}
