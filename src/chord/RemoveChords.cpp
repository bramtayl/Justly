#include "chord/RemoveChords.hpp"

#include <QtGlobal>

#include "chord/ChordsModel.hpp"

RemoveChords::RemoveChords(ChordsModel *chords_model_pointer_input,
                           qsizetype first_chord_number_input,
                           const QList<Chord> &old_chords_input,
                           QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_chord_number(first_chord_number_input),
      old_chords(old_chords_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

auto RemoveChords::undo() -> void {
  insert_chords(*chords_model_pointer, first_chord_number, old_chords);
}

auto RemoveChords::redo() -> void {
  remove_chords(*chords_model_pointer, first_chord_number, old_chords.size());
}
