#pragma once

#include <QString>
#include <QUndoStack>
#include <cstddef>

struct NotesModel;

struct SetNoteWords : public QUndoCommand {
  NotesModel *const notes_model_pointer;
  const size_t note_number;
  const QString old_words;
  const QString new_words;

  explicit SetNoteWords(NotesModel *notes_model_pointer_input,
                        size_t note_number_input,
                        const QString &old_words_input,
                        const QString &new_words_input,
                        QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
