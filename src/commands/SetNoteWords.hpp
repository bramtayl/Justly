#pragma once

#include <QString>
#include <QUndoStack>
#include <cstddef>

struct ChordsModel;

class SetNoteWords : public QUndoCommand {
private:
  ChordsModel *const chords_model_pointer;
  const size_t chord_number;
  const size_t note_number;
  const QString old_words;
  const QString new_words;

public:
  explicit SetNoteWords(ChordsModel *chords_model_pointer_input,
                        size_t chord_number_input, size_t note_number_input,
                        QString old_words_input,
                        QString new_words_input,
                        QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};