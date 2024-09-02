#include "commands/SetNotesCells.hpp"

#include <QString>
#include <QtGlobal>
#include <cstddef>
#include <variant>

#include "note_chord/Chord.hpp"
#include "note_chord/Note.hpp"
#include "other/ChordsModel.hpp"
#include "other/templates.hpp"
#include "rational/Rational.hpp"

static void replace_note_cells(ChordsModel *chords_model_pointer,
                               size_t chord_number, size_t first_note_number,
                               NoteChordColumn left_column,
                               NoteChordColumn right_column,
                               const std::vector<Note> &new_notes) {
  Q_ASSERT(chords_model_pointer != nullptr);
  auto &notes = get_item(chords_model_pointer->chords, chord_number).notes;
  auto number_of_notes = new_notes.size();
  for (size_t replace_number = 0; replace_number < number_of_notes;
       replace_number = replace_number + 1) {
    auto &note = get_item(notes, first_note_number + replace_number);
    const auto &new_note = get_const_item(new_notes, replace_number);
    for (auto note_chord_column = left_column;
         note_chord_column <= right_column;
         note_chord_column =
             static_cast<NoteChordColumn>(note_chord_column + 1)) {
      switch (note_chord_column) {
      case instrument_column:
        note.instrument_pointer = new_note.instrument_pointer;
        break;
      case interval_or_percussion_column:
        note.interval_or_percussion_pointer =
            new_note.interval_or_percussion_pointer;
        break;
      case beats_column:
        note.beats = new_note.beats;
        break;
      case velocity_ratio_column:
        note.velocity_ratio = new_note.velocity_ratio;
        break;
      case tempo_ratio_column:
        note.tempo_ratio = new_note.tempo_ratio;
        break;
      case words_column:
        note.words = new_note.words;
        break;
      default:
        Q_ASSERT(false);
        break;
      }
    }
  }
  chords_model_pointer->edited_notes_cells(chord_number, first_note_number,
                                           number_of_notes, left_column,
                                           right_column);
}

SetNotesCells::SetNotesCells(ChordsModel *chords_model_pointer_input,
                             size_t chord_number_input,
                             size_t first_note_number_input,
                             NoteChordColumn left_column_input,
                             NoteChordColumn right_column_input,
                             const std::vector<Note> &old_notes_input,
                             const std::vector<Note> &new_notes_input,
                             QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input),
      first_note_number(first_note_number_input),
      left_column(left_column_input), right_column(right_column_input),
      old_notes(old_notes_input), new_notes(new_notes_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetNotesCells::undo() {
  replace_note_cells(chords_model_pointer, chord_number, first_note_number,
                     left_column, right_column, old_notes);
}

void SetNotesCells::redo() {
  replace_note_cells(chords_model_pointer, chord_number, first_note_number,
                     left_column, right_column, old_notes);
}
