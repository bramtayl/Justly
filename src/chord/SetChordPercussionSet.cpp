#include "chord/SetChordPercussionSet.hpp"

#include <QList>
#include <QtGlobal>

#include "chord/Chord.hpp"
#include "chord/ChordsModel.hpp"
#include "justly/ChordColumn.hpp"

static void set_chord_percussion_set_pointer(
    ChordsModel &chords_model, qsizetype chord_number,
    const PercussionSet *new_percussion_set_pointer) {
  chords_model.chords[chord_number].percussion_set_pointer =
      new_percussion_set_pointer;
  chords_model.edited_cells(chord_number, 1, chord_percussion_set_column,
                            chord_percussion_set_column);
}

SetChordPercussionSet::SetChordPercussionSet(
    ChordsModel *chords_model_pointer_input, qsizetype chord_number_input,
    const PercussionSet *old_percussion_set_pointer_input,
    const PercussionSet *new_percussion_set_pointer_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input),
      old_percussion_set_pointer(old_percussion_set_pointer_input),
      new_percussion_set_pointer(new_percussion_set_pointer_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetChordPercussionSet::undo() {
  set_chord_percussion_set_pointer(*chords_model_pointer, chord_number,
                                   old_percussion_set_pointer);
}

void SetChordPercussionSet::redo() {
  set_chord_percussion_set_pointer(*chords_model_pointer, chord_number,
                                   new_percussion_set_pointer);
}
