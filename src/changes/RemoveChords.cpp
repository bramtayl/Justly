#include "changes/RemoveChords.hpp"

#include <QtGlobal>

#include "justly/Chord.hpp"
#include "justly/ChordsModel.hpp"

RemoveChords::RemoveChords(ChordsModel* chords_model_pointer_input,
                                       size_t first_child_number_input,
                                       const std::vector<Chord>& chords_input,
                                       QUndoCommand* parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_child_number(first_child_number_input),
      chords(chords_input) {}

auto RemoveChords::undo() -> void {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->insert_chords_directly(first_child_number, chords);
}

auto RemoveChords::redo() -> void {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->remove_chords_directly(first_child_number, chords.size());
}
