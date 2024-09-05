#include "chord/InsertChords.hpp"

#include <QtGlobal>

#include "chord/ChordsModel.hpp"

struct Chord;

InsertChords::InsertChords(ChordsModel *chords_model_pointer_input,
                           qsizetype first_chord_number_input,
                           const QList<Chord> &new_chords_input,
                           QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_chord_number(first_chord_number_input),
      new_chords(new_chords_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

auto InsertChords::undo() -> void {
  remove_chords(chords_model_pointer, first_chord_number, new_chords.size());
}

auto InsertChords::redo() -> void {
  insert_chords(chords_model_pointer, first_chord_number, new_chords);
}
