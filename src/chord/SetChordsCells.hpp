#pragma once

#include <QUndoStack>
#include <QtGlobal>
#include <QList>

#include "justly/ChordColumn.hpp"

struct Chord;
struct ChordsModel;

struct SetChordsCells : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const qsizetype first_chord_number;
  const ChordColumn left_column;
  const ChordColumn right_column;
  const QList<Chord> old_chords;
  const QList<Chord> new_chords;
  explicit SetChordsCells(ChordsModel *chords_model_pointer_input,
                          qsizetype first_chord_number_input,
                          ChordColumn left_column_input,
                          ChordColumn right_column_input,
                          const QList<Chord> &old_chords_input,
                          const QList<Chord> &new_chords_input,
                          QUndoCommand *parent_pointer_input = nullptr);
  void undo() override;
  void redo() override;
};
