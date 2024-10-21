#include "chord/SetChordInstrument.hpp"

#include <QList>
#include <QtGlobal>

#include "chord/Chord.hpp"
#include "chord/ChordsModel.hpp"
#include "justly/ChordColumn.hpp"

static void
set_chord_instrument_pointer(ChordsModel &chords_model, qsizetype chord_number,
                             const Instrument *new_instrument_pointer) {
  chords_model.chords[chord_number].instrument_pointer = new_instrument_pointer;
  chords_model.edited_cells(chord_number, 1, chord_instrument_column,
                            chord_instrument_column);
}

SetChordInstrument::SetChordInstrument(
    ChordsModel *chords_model_pointer_input, qsizetype chord_number_input,
    const Instrument *old_instrument_pointer_input,
    const Instrument *new_instrument_pointer_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input),
      old_instrument_pointer(old_instrument_pointer_input),
      new_instrument_pointer(new_instrument_pointer_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetChordInstrument::undo() {
  set_chord_instrument_pointer(*chords_model_pointer, chord_number,
                               old_instrument_pointer);
}

void SetChordInstrument::redo() {
  set_chord_instrument_pointer(*chords_model_pointer, chord_number,
                               new_instrument_pointer);
}
