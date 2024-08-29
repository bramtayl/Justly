#pragma once

#include <QUndoStack>
#include <cstddef>

#include "interval/Interval.hpp"

struct ChordsModel;
struct Instrument;
struct Percussion;

struct ChangeToInterval : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  size_t chord_number;
  size_t note_number;
  const Instrument *const old_instrument_pointer;
  const Instrument *const new_instrument_pointer;
  const Percussion *const old_percussion_pointer;
  const Interval new_interval;

  explicit ChangeToInterval(ChordsModel *chords_model_pointer_input,
                            size_t chord_number_input, size_t note_number_input,
                            const Instrument *old_instrument_pointer_input,
                            const Instrument *new_instrument_pointer_input,
                            const Percussion *old_percussion_pointer_input,
                            const Interval &new_interval_input,
                            QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
