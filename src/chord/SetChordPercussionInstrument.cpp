#include "chord/SetChordPercussionInstrument.hpp"

#include <QList>
#include <QtGlobal>

#include "chord/Chord.hpp"
#include "chord/ChordsModel.hpp"
#include "justly/ChordColumn.hpp"

static void set_chord_percussion_instrument_pointer(
    ChordsModel &chords_model, qsizetype chord_number,
    const PercussionInstrument *new_percussion_instrument_pointer) {
  chords_model.chords[chord_number].percussion_instrument_pointer =
      new_percussion_instrument_pointer;
  chords_model.edited_cells(chord_number, 1, chord_percussion_instrument_column,
                            chord_percussion_instrument_column);
}

SetChordPercussionInstrument::SetChordPercussionInstrument(
    ChordsModel *chords_model_pointer_input, qsizetype chord_number_input,
    const PercussionInstrument *old_percussion_instrument_pointer_input,
    const PercussionInstrument *new_percussion_instrument_pointer_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input),
      old_percussion_instrument_pointer(
          old_percussion_instrument_pointer_input),
      new_percussion_instrument_pointer(
          new_percussion_instrument_pointer_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetChordPercussionInstrument::undo() {
  set_chord_percussion_instrument_pointer(*chords_model_pointer, chord_number,
                                          old_percussion_instrument_pointer);
}

void SetChordPercussionInstrument::redo() {
  set_chord_percussion_instrument_pointer(*chords_model_pointer, chord_number,
                                          new_percussion_instrument_pointer);
}
