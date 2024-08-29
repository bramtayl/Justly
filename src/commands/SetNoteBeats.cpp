#include "commands/SetNoteBeats.hpp"

#include <QtGlobal>

#include "justly/NoteChordColumn.hpp"
#include "note_chord/Chord.hpp"
#include "note_chord/Note.hpp"
#include "other/ChordsModel.hpp"
#include "other/templates.hpp"

static void set_note_beats(ChordsModel *chords_model_pointer,
                           size_t chord_number, size_t note_number,
                           const Rational &new_beats) {
  Q_ASSERT(chords_model_pointer != nullptr);
  get_item(get_item(chords_model_pointer->chords, chord_number).notes,
           note_number)
      .beats = new_beats;
  chords_model_pointer->edited_notes_cells(chord_number, note_number, 1,
                                           beats_column, beats_column);
}

SetNoteBeats::SetNoteBeats(ChordsModel *chords_model_pointer_input,
                           size_t chord_number_input, size_t note_number_input,
                           const Rational &old_beats_input,
                           const Rational &new_beats_input,
                           QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), note_number(note_number_input),
      old_beats(old_beats_input), new_beats(new_beats_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetNoteBeats::undo() {
  set_note_beats(chords_model_pointer, chord_number, note_number, old_beats);
}

void SetNoteBeats::redo() {
  set_note_beats(chords_model_pointer, chord_number, note_number, new_beats);
}
