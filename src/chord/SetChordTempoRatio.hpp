#pragma once

#include <QUndoStack>
#include <QtGlobal>

#include "rational/Rational.hpp"

struct ChordsModel;

struct SetChordTempoRatio : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const qsizetype chord_number;
  const Rational old_tempo;
  const Rational new_tempo;

  explicit SetChordTempoRatio(ChordsModel *chords_model_pointer_input,
                              qsizetype chord_number_input,
                              const Rational &old_tempo_input,
                              const Rational &new_tempo_input,
                              QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
