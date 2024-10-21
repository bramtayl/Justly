#pragma once

#include <QUndoStack>
#include <QtGlobal>

struct ChordsModel;
struct PercussionInstrument;

struct SetChordPercussionInstrument : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const qsizetype chord_number;
  const PercussionInstrument *old_percussion_instrument_pointer;
  const PercussionInstrument *new_percussion_instrument_pointer;

  explicit SetChordPercussionInstrument(
      ChordsModel *chords_model_pointer_input, qsizetype chord_number_input,
      const PercussionInstrument *old_percussion_instrument_pointer_input,
      const PercussionInstrument *new_percussion_instrument_pointer_input,
      QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
