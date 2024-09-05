#include "chord/SetChordWords.hpp"

#include <QtGlobal>

#include "chord/Chord.hpp"
#include "chord/ChordsModel.hpp"
#include "justly/ChordColumn.hpp"
#include "other/templates.hpp"

static void set_chord_words(ChordsModel *chords_model_pointer,
                            size_t chord_number, const QString &new_words) {
  Q_ASSERT(chords_model_pointer != nullptr);
  get_item(chords_model_pointer->chords, chord_number).words = new_words;
  chords_model_pointer->edited_chords_cells(chord_number, 1, chord_words_column,
                                            chord_words_column);
}

SetChordWords::SetChordWords(ChordsModel *chords_model_pointer_input,
                             size_t chord_number_input,
                             const QString &old_words_input,
                             const QString &new_words_input,
                             QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), old_words(old_words_input),
      new_words(new_words_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetChordWords::undo() {
  set_chord_words(chords_model_pointer, chord_number, old_words);
}

void SetChordWords::redo() {
  set_chord_words(chords_model_pointer, chord_number, new_words);
}
