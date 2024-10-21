#pragma once

#include <QUndoStack>
#include <QtGlobal>

struct ChordsModel;
struct Instrument;

struct SetChordInstrument : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const qsizetype chord_number;
  const Instrument *old_instrument_pointer;
  const Instrument *new_instrument_pointer;

  explicit SetChordInstrument(ChordsModel *chords_model_pointer_input,
                              qsizetype chord_number_input,
                              const Instrument *old_instrument_pointer_input,
                              const Instrument *new_instrument_pointer_input,
                              QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
