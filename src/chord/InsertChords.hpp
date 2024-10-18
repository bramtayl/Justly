#pragma once

#include <QList>
#include <QUndoStack>
#include <QtGlobal>

struct Chord;
struct ChordsModel;

struct InsertChords : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const qsizetype first_chord_number;
  const QList<Chord> new_chords;

  InsertChords(ChordsModel *chords_model_pointer_input,
               qsizetype first_chord_number_input,
               const QList<Chord> &new_chords_input,
               QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
