#include "commands/SetNoteInstrument.hpp"

#include <QtGlobal>

#include "justly/NoteChordColumn.hpp"
#include "note_chord/Chord.hpp"
#include "note_chord/Note.hpp"
#include "other/ChordsModel.hpp"
#include "other/templates.hpp"

static void set_note_instrument(ChordsModel *chords_model_pointer,
                                size_t chord_number, size_t note_number,
                                const Instrument *new_instrument_pointer) {
  Q_ASSERT(chords_model_pointer != nullptr);
  get_item(get_item(chords_model_pointer->chords, chord_number).notes,
           note_number)
      .instrument_pointer = new_instrument_pointer;
  chords_model_pointer->edited_notes_cells(
      chord_number, note_number, 1, instrument_column, instrument_column);
}

SetNoteInstrument::SetNoteInstrument(
    ChordsModel *chords_model_pointer_input, size_t chord_number_input,
    size_t note_number_input, const Instrument *old_instrument_pointer_input,
    const Instrument *new_instrument_pointer_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), note_number(note_number_input),
      old_instrument_pointer(old_instrument_pointer_input),
      new_instrument_pointer(new_instrument_pointer_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetNoteInstrument::undo() {
  set_note_instrument(chords_model_pointer, chord_number, note_number,
                      old_instrument_pointer);
}

void SetNoteInstrument::redo() {
  set_note_instrument(chords_model_pointer, chord_number, note_number,
                      new_instrument_pointer);
}
