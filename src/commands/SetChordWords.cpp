#include "commands/SetChordWords.hpp"

#include <QtGlobal>
#include <utility>

#include "other/ChordsModel.hpp"

SetChordWords::SetChordWords(ChordsModel *chords_model_pointer_input,
                         size_t chord_number_input,
                         QString old_words_input, QString new_words_input, 
                         QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input),
      old_words(std::move(old_words_input)),
      new_words(std::move(new_words_input)) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetChordWords::undo() {
  chords_model_pointer->set_chord_words(chord_number, old_words);
}

void SetChordWords::redo() {
  chords_model_pointer->set_chord_words(chord_number, new_words);
}
