#include "commands/SetChordsCells.hpp"

#include <QString>
#include <QtGlobal>
#include <cstddef>

#include "interval/Interval.hpp"
#include "note_chord/Chord.hpp"
#include "other/ChordsModel.hpp"
#include "other/templates.hpp"
#include "rational/Rational.hpp"

static void replace_chord_cells(ChordsModel *chords_model_pointer,
                                size_t first_chord_number,
                                NoteChordColumn left_column,
                                NoteChordColumn right_column,
                                const std::vector<Chord> &new_chords) {
  Q_ASSERT(chords_model_pointer != nullptr);
  auto &chords = chords_model_pointer->chords;
  auto number_of_chords = new_chords.size();
  for (size_t replace_number = 0; replace_number < number_of_chords;
       replace_number++) {
    auto &chord = get_item(chords, first_chord_number + replace_number);
    const auto &new_chord = get_const_item(new_chords, replace_number);
    for (auto note_chord_column = left_column;
         note_chord_column <= right_column;
         note_chord_column =
             static_cast<NoteChordColumn>(note_chord_column + 1)) {
      switch (note_chord_column) {
      case instrument_column:
        break;
      case interval_or_percussion_column:
        chord.interval =
            new_chord.interval;
        break;
      case beats_column:
        chord.beats = new_chord.beats;
        break;
      case velocity_ratio_column:
        chord.velocity_ratio = new_chord.velocity_ratio;
        break;
      case tempo_ratio_column:
        chord.tempo_ratio = new_chord.tempo_ratio;
        break;
      case words_column:
        chord.words = new_chord.words;
        break;
      default:
        Q_ASSERT(false);
        break;
      }
    }
  }
  chords_model_pointer->edited_chords_cells(
      first_chord_number, number_of_chords, left_column, right_column);
}

SetChordsCells::SetChordsCells(ChordsModel *chords_model_pointer_input,
                               size_t first_chord_number_input,
                               NoteChordColumn left_column_input,
                               NoteChordColumn right_column_input,
                               const std::vector<Chord> &old_chords_input,
                               const std::vector<Chord> &new_chords_input,
                               QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_chord_number(first_chord_number_input),
      left_column(left_column_input), right_column(right_column_input),
      old_chords(old_chords_input), new_chords(new_chords_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetChordsCells::undo() {
  replace_chord_cells(chords_model_pointer, first_chord_number, left_column,
                      right_column, old_chords);
}

void SetChordsCells::redo() {
  replace_chord_cells(chords_model_pointer, first_chord_number, left_column,
                      right_column, new_chords);
}
