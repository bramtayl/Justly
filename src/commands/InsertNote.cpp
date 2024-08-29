#include "commands/InsertNote.hpp"

#include <QtGlobal>
#include <vector>

#include "note_chord/Chord.hpp"
#include "other/ChordsModel.hpp"
#include "other/templates.hpp"

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
  remove_notes(chords_model_pointer, chord_number, note_number, 1);
}

auto InsertNote::redo() -> void {
  Q_ASSERT(chords_model_pointer != nullptr);
  auto &notes = get_item(chords_model_pointer->chords, chord_number).notes;
  check_end_number(notes, note_number);

  chords_model_pointer->begin_insert_notes(chord_number, note_number, 1);
  notes.insert(notes.begin() + static_cast<int>(note_number), new_note);
  chords_model_pointer->end_insert_rows();
}
