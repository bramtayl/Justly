#include "changes/SetNoteCell.hpp"

#include <QVariant>
#include <QtGlobal>
#include <utility>

#include "models/ChordsModel.hpp"

SetNoteCell::SetNoteCell(ChordsModel *chords_model_pointer_input,
                         size_t chord_number_input, size_t note_number_input,
                         NoteChordColumn note_chord_column_input,
                         QVariant old_value_input, QVariant new_value_input,
                         QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), note_number(note_number_input),
      note_chord_column(note_chord_column_input),
      old_value(std::move(old_value_input)),
      new_value(std::move(new_value_input)) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetNoteCell::undo() {
  chords_model_pointer->set_note_cell(chord_number, note_number,
                                      note_chord_column, old_value);
}

void SetNoteCell::redo() {
  chords_model_pointer->set_note_cell(chord_number, note_number,
                                      note_chord_column, new_value);
}
