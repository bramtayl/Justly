#include "commands/RemoveChords.hpp"

#include <QtGlobal>

#include "models/ChordsModel.hpp"
#include "note_chord/Chord.hpp"

RemoveChords::RemoveChords(ChordsModel *chords_model_pointer_input,
                           size_t first_chord_number_input,
                           const std::vector<Chord> &old_chords_input,
                           QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_chord_number(first_chord_number_input),
      old_chords(old_chords_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

auto RemoveChords::undo() -> void {
  chords_model_pointer->insert_chords(first_chord_number, old_chords);
}

auto RemoveChords::redo() -> void {
  chords_model_pointer->remove_chords(first_chord_number, old_chords.size());
}
