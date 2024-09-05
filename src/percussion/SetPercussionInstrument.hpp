#pragma once

#include <QUndoStack>
#include <cstddef>

struct PercussionsModel;
struct PercussionInstrument;
struct Instrument;

struct SetPercussionInstrument : public QUndoCommand {
  PercussionsModel *const percussions_model_pointer;
  const size_t percussion_number;
  const PercussionInstrument *const old_percussion_instrument_pointer;
  const PercussionInstrument *const new_percussion_instrument_pointer;

  explicit SetPercussionInstrument(
      PercussionsModel *percussions_model_pointer_input,
      size_t percussion_number_input,
      const PercussionInstrument *old_percussion_instrument_pointer_input,
      const PercussionInstrument *new_percussion_instrument_pointer_input,
      QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
