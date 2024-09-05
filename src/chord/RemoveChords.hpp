#pragma once

#include <QUndoStack>
#include <QtGlobal>
#include <QList>

struct Chord;
struct ChordsModel;

struct RemoveChords : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const qsizetype first_chord_number;
  const QList<Chord> old_chords;

  RemoveChords(ChordsModel *chords_model_pointer_input,
               qsizetype first_chord_number_input,
               const QList<Chord> &old_chords_input,
               QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
