#include "commands/SetNoteWords.hpp"

#include <QtGlobal>
#include <utility>

#include "justly/NoteChordColumn.hpp"
#include "note_chord/Chord.hpp"
#include "note_chord/Note.hpp"
#include "other/ChordsModel.hpp"
#include "other/templates.hpp"

static void set_note_words(ChordsModel *chords_model_pointer,
                           size_t chord_number, size_t note_number,
                           const QString &new_words) {
  Q_ASSERT(chords_model_pointer != nullptr);
  get_item(get_item(chords_model_pointer->chords, chord_number).notes,
           note_number)
      .words = new_words;
  chords_model_pointer->edited_notes_cells(chord_number, note_number, 1,
                                           words_column, words_column);
}

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
  set_note_words(chords_model_pointer, chord_number, note_number, old_words);
}

void SetNoteWords::redo() {
  set_note_words(chords_model_pointer, chord_number, note_number, new_words);
}
