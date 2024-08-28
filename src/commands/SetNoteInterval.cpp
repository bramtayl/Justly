#include "commands/SetNoteInterval.hpp"

#include <QtGlobal>

#include "other/ChordsModel.hpp"

SetNoteInterval::SetNoteInterval(ChordsModel *chords_model_pointer_input,
                                 size_t chord_number_input,
                                 size_t note_number_input,
                                 const Interval &old_interval_input,
                                 const Interval &new_interval_input,
                                 QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), note_number(note_number_input),
      old_interval(old_interval_input), new_interval(new_interval_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetNoteInterval::undo() {
  chords_model_pointer->set_note_interval(chord_number, note_number,
                                          old_interval);
}

void SetNoteInterval::redo() {
  chords_model_pointer->set_note_interval(chord_number, note_number,
                                          new_interval);
}
