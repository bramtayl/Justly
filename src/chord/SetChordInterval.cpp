#include "chord/SetChordInterval.hpp"

#include <QtGlobal>
#include <QList>

#include "chord/Chord.hpp" // for Chord
#include "chord/ChordsModel.hpp"
#include "justly/ChordColumn.hpp" // for ChordColumn


static void set_chord_interval(ChordsModel *chords_model_pointer,
                               qsizetype chord_number,
                               const Interval &new_interval) {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->chords[chord_number].interval = new_interval;
  chords_model_pointer->edited_chords_cells(
      chord_number, 1, chord_interval_column, chord_interval_column);
}

SetChordInterval::SetChordInterval(ChordsModel *chords_model_pointer_input,
                                   qsizetype chord_number_input,
                                   const Interval &old_interval_input,
                                   const Interval &new_interval_input,
                                   QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), old_interval(old_interval_input),
      new_interval(new_interval_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetChordInterval::undo() {
  set_chord_interval(chords_model_pointer, chord_number, old_interval);
}

void SetChordInterval::redo() {
  set_chord_interval(chords_model_pointer, chord_number, new_interval);
}
