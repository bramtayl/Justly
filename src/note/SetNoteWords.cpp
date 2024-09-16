#include "note/SetNoteWords.hpp"

#include <QtGlobal>
#include <QList>
#include <utility>

#include "justly/NoteColumn.hpp"
#include "note/Note.hpp"
#include "note/NotesModel.hpp"

static void set_note_words(NotesModel& notes_model, qsizetype note_number,
                           const QString &new_words) {
  auto *notes_pointer = notes_model.notes_pointer;
  Q_ASSERT(notes_pointer != nullptr);
  (*notes_pointer)[note_number].words = new_words;
  notes_model.edited_cells(note_number, 1, note_words_column,
                                          note_words_column);
}

SetNoteWords::SetNoteWords(NotesModel *notes_model_pointer_input,
                           qsizetype note_number_input,
                           QString old_words_input,
                           QString new_words_input,
                           QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      notes_model_pointer(notes_model_pointer_input),
      note_number(note_number_input), old_words(std::move(old_words_input)),
      new_words(std::move(new_words_input)) {
  Q_ASSERT(notes_model_pointer != nullptr);
}

void SetNoteWords::undo() {
  set_note_words(*notes_model_pointer, note_number, old_words);
}

void SetNoteWords::redo() {
  set_note_words(*notes_model_pointer, note_number, new_words);
}
