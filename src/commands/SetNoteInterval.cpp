#include "commands/SetNoteInterval.hpp"

#include <QtGlobal>
#include <variant>

#include "justly/NoteChordColumn.hpp"
#include "note_chord/Chord.hpp"
#include "note_chord/Note.hpp"
#include "other/ChordsModel.hpp"
#include "other/templates.hpp"

static void set_note_interval(ChordsModel *chords_model_pointer,
                              size_t chord_number, size_t note_number,
                              const Interval &new_interval) {
  Q_ASSERT(chords_model_pointer != nullptr);
  get_item(get_item(chords_model_pointer->chords, chord_number).notes,
           note_number)
      .interval_or_percussion_pointer = new_interval;
  chords_model_pointer->edited_notes_cells(chord_number, note_number, 1,
                                           interval_or_percussion_column,
                                           interval_or_percussion_column);
}

SetNoteInterval::SetNoteInterval(ChordsModel *chords_model_pointer_input,
                                 size_t chord_number_input,
                                 size_t note_number_input,
                                 const Interval &old_interval_input,
                                 const Interval &new_interval_input,
                                 QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), note_number(note_number_input),
      old_interval(old_interval_input), new_interval(new_interval_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetNoteInterval::undo() {
  set_note_interval(chords_model_pointer, chord_number, note_number,
                    old_interval);
}

void SetNoteInterval::redo() {
  set_note_interval(chords_model_pointer, chord_number, note_number,
                    new_interval);
}
