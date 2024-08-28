#include "commands/InsertChord.hpp"

#include <QtGlobal>

#include "other/ChordsModel.hpp"

InsertChord::InsertChord(ChordsModel *chords_model_pointer_input,
                         size_t chord_number_input,
                         const Chord& new_chord_input,
                         QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input),
      new_chord(new_chord_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

auto InsertChord::undo() -> void {
  chords_model_pointer->remove_chords(chord_number, 1);
}

auto InsertChord::redo() -> void {
  chords_model_pointer->insert_chord(chord_number, new_chord);
}
