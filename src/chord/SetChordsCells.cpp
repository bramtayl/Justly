#include "chord/SetChordsCells.hpp"

#include <QString>
#include <QtGlobal>

#include "chord/Chord.hpp"
#include "chord/ChordsModel.hpp"
#include "interval/Interval.hpp"
#include "note/Note.hpp"
#include "percussion/Percussion.hpp"
#include "rational/Rational.hpp"

static void replace_chord_cells(ChordsModel &chords_model,
                                qsizetype first_chord_number,
                                ChordColumn left_column,
                                ChordColumn right_column,
                                const QList<Chord> &new_chords) {
  auto &chords = chords_model.chords;
  auto number_of_chords = new_chords.size();
  for (qsizetype replace_number = 0; replace_number < number_of_chords;
       replace_number++) {
    auto &chord = chords[first_chord_number + replace_number];
    const auto &new_chord = new_chords.at(replace_number);
    for (auto chord_column = left_column; chord_column <= right_column;
         chord_column = static_cast<ChordColumn>(chord_column + 1)) {
      switch (chord_column) {
      case chord_interval_column:
        chord.interval = new_chord.interval;
        break;
      case chord_beats_column:
        chord.beats = new_chord.beats;
        break;
      case chord_velocity_ratio_column:
        chord.velocity_ratio = new_chord.velocity_ratio;
        break;
      case chord_tempo_ratio_column:
        chord.tempo_ratio = new_chord.tempo_ratio;
        break;
      case chord_words_column:
        chord.words = new_chord.words;
        break;
      case chord_notes_column:
        chord.notes = new_chord.notes;
        break;
      case chord_percussions_column:
        chord.percussions = new_chord.percussions;
        break;
      default:
        Q_ASSERT(false);
        break;
      }
    }
  }
  chords_model.edited_cells(first_chord_number, number_of_chords, left_column,
                            right_column);
}

SetChordsCells::SetChordsCells(ChordsModel *chords_model_pointer_input,
                               qsizetype first_chord_number_input,
                               ChordColumn left_column_input,
                               ChordColumn right_column_input,
                               const QList<Chord> &old_chords_input,
                               const QList<Chord> &new_chords_input,
                               QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_chord_number(first_chord_number_input),
      left_column(left_column_input), right_column(right_column_input),
      old_chords(old_chords_input), new_chords(new_chords_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetChordsCells::undo() {
  replace_chord_cells(*chords_model_pointer, first_chord_number, left_column,
                      right_column, old_chords);
}

void SetChordsCells::redo() {
  replace_chord_cells(*chords_model_pointer, first_chord_number, left_column,
                      right_column, new_chords);
}
