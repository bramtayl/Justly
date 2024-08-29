#include "commands/SetNotePercussion.hpp"

#include <QtGlobal>
#include <variant>

#include "justly/NoteChordColumn.hpp"
#include "note_chord/Chord.hpp"
#include "note_chord/Note.hpp"
#include "other/ChordsModel.hpp"
#include "other/templates.hpp"

struct Percussion;

static void set_note_percussion(ChordsModel *chords_model_pointer,
                                size_t chord_number, size_t note_number,
                                const Percussion *new_percussion_pointer) {
  Q_ASSERT(chords_model_pointer != nullptr);
  get_item(get_item(chords_model_pointer->chords, chord_number).notes,
           note_number)
      .interval_or_percussion_pointer = new_percussion_pointer;
  chords_model_pointer->edited_notes_cells(chord_number, note_number, 1,
                                           interval_or_percussion_column,
                                           interval_or_percussion_column);
}

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
  set_note_percussion(chords_model_pointer, chord_number, note_number,
                      old_percussion_pointer);
}

void SetNotePercussion::redo() {
  set_note_percussion(chords_model_pointer, chord_number, note_number,
                      new_percussion_pointer);
}
