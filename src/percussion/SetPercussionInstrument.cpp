#include "percussion/SetPercussionInstrument.hpp"

#include <QtGlobal>

#include "chord/Chord.hpp"
#include "justly/ChordColumn.hpp"
#include "justly/PercussionColumn.hpp"
#include "note/Note.hpp"
#include "other/templates.hpp"
#include "percussion/PercussionsModel.hpp"

static void set_percussion_instrument(
    PercussionsModel *percussions_model_pointer, size_t percussion_number,
    const PercussionInstrument *new_percussion_instrument_pointer) {
  Q_ASSERT(percussions_model_pointer != nullptr);
  get_item(percussions_model_pointer->percussions, percussion_number)
      .percussion_instrument_pointer = new_percussion_instrument_pointer;
  percussions_model_pointer->edited_percussions_cells(
      percussion_number, 1, percussion_instrument_column,
      percussion_instrument_column);
}

SetPercussionInstrument::SetPercussionInstrument(
    PercussionsModel *percussions_model_pointer_input,
    size_t percussion_number_input,
    const PercussionInstrument *old_percussion_instrument_pointer_input,
    const PercussionInstrument *new_percussion_instrument_pointer_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      percussions_model_pointer(percussions_model_pointer_input),
      percussion_number(percussion_number_input),
      old_percussion_instrument_pointer(
          old_percussion_instrument_pointer_input),
      new_percussion_instrument_pointer(
          new_percussion_instrument_pointer_input) {
  Q_ASSERT(percussions_model_pointer != nullptr);
}

void SetPercussionInstrument::undo() {
  set_percussion_instrument(percussions_model_pointer, percussion_number,
                            old_percussion_instrument_pointer);
}

void SetPercussionInstrument::redo() {
  set_percussion_instrument(percussions_model_pointer, percussion_number,
                            new_percussion_instrument_pointer);
}
