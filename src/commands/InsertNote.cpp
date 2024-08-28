#include "commands/InsertNote.hpp"

#include <QtGlobal>

#include "other/ChordsModel.hpp"

struct Note;

InsertNote::InsertNote(ChordsModel *chords_model_pointer_input,
                       size_t chord_number_input, size_t note_number_input,
                       const Note &new_note_input,
                       QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), note_number(note_number_input),
      new_note(new_note_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

auto InsertNote::undo() -> void {
  chords_model_pointer->remove_notes(chord_number, note_number, 1);
}

auto InsertNote::redo() -> void {
  chords_model_pointer->insert_note(chord_number, note_number, new_note);
}
