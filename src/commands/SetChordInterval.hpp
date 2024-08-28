#pragma once

#include <QUndoStack>
#include <cstddef>

#include "interval/Interval.hpp"

struct ChordsModel;

class SetChordInterval : public QUndoCommand {
private:
  ChordsModel *const chords_model_pointer;
  const size_t chord_number;
  const Interval old_interval;
  const Interval new_interval;

public:
  explicit SetChordInterval(ChordsModel *chords_model_pointer_input,
                           size_t chord_number_input,
                           const Interval &old_interval_input,
                           const Interval &new_interval_input,
                           QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};