#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "interval/Interval.hpp"

struct ChordsModel;

struct SetChordInterval : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const qsizetype chord_number;
  const Interval old_interval;
  const Interval new_interval;

  explicit SetChordInterval(ChordsModel *chords_model_pointer_input,
                            qsizetype chord_number_input,
                            const Interval &old_interval_input,
                            const Interval &new_interval_input,
                            QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
