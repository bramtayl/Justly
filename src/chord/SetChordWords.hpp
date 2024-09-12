#pragma once

#include <QString>
#include <QUndoStack>
#include <QtGlobal>

struct ChordsModel;

struct SetChordWords : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const qsizetype chord_number;
  const QString old_words;
  const QString new_words;

  explicit SetChordWords(ChordsModel *chords_model_pointer_input,
                         qsizetype chord_number_input,
                         QString old_words_input,
                         QString new_words_input,
                         QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
