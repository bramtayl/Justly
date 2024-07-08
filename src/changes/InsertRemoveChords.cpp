#include "changes/InsertRemoveChords.hpp"

#include <qassert.h>  // for Q_ASSERT

#include "justly/Chord.hpp"        // for Chord
#include "justly/ChordsModel.hpp"  // for ChordsModel

InsertRemoveChords::InsertRemoveChords(ChordsModel* chords_model_pointer_input,
                                       size_t first_child_number_input,
                                       const std::vector<Chord>& chords_input,
                                       bool is_insert_input,
                                       QUndoCommand* parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_child_number(first_child_number_input),
      chords(chords_input),
      is_insert(is_insert_input) {}

auto InsertRemoveChords::undo() -> void { insert_or_remove(!is_insert); }

auto InsertRemoveChords::redo() -> void { insert_or_remove(is_insert); }

void InsertRemoveChords::insert_or_remove(bool should_insert) {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->insert_remove_chords(first_child_number, chords,
                                             should_insert);
}
