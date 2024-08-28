#include "commands/SetNoteWords.hpp"

#include <QtGlobal>
#include <utility>

#include "other/ChordsModel.hpp"

SetNoteWords::SetNoteWords(ChordsModel *chords_model_pointer_input,
                         size_t chord_number_input, size_t note_number_input,
                         QString old_words_input, QString new_words_input, 
                         QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), note_number(note_number_input),
      old_words(std::move(old_words_input)),
      new_words(std::move(new_words_input)) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetNoteWords::undo() {
  chords_model_pointer->set_note_words(chord_number, note_number, old_words);
}

void SetNoteWords::redo() {
  chords_model_pointer->set_note_words(chord_number, note_number, new_words);
}
