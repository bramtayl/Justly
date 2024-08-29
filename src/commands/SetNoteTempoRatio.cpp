#include "commands/SetNoteTempoRatio.hpp"

#include <QtGlobal>

#include "justly/NoteChordColumn.hpp"
#include "note_chord/Chord.hpp"
#include "note_chord/Note.hpp"
#include "other/ChordsModel.hpp"
#include "other/templates.hpp"

static void set_note_tempo_ratio(ChordsModel *chords_model_pointer,
                                 size_t chord_number, size_t note_number,
                                 const Rational &new_tempo_ratio) {
  Q_ASSERT(chords_model_pointer != nullptr);
  get_item(get_item(chords_model_pointer->chords, chord_number).notes,
           note_number)
      .tempo_ratio = new_tempo_ratio;
  chords_model_pointer->edited_notes_cells(
      chord_number, note_number, 1, tempo_ratio_column, tempo_ratio_column);
}

SetNoteTempoRatio::SetNoteTempoRatio(ChordsModel *chords_model_pointer_input,
                                     size_t chord_number_input,
                                     size_t note_number_input,
                                     const Rational &old_tempo_input,
                                     const Rational &new_tempo_input,
                                     QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), note_number(note_number_input),
      old_tempo(old_tempo_input), new_tempo(new_tempo_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetNoteTempoRatio::undo() {
  set_note_tempo_ratio(chords_model_pointer, chord_number, note_number,
                       old_tempo);
}

void SetNoteTempoRatio::redo() {
  set_note_tempo_ratio(chords_model_pointer, chord_number, note_number,
                       new_tempo);
}
