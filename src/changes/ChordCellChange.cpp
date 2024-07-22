#include "changes/ChordCellChange.hpp"

#include <QVariant>
#include <QtGlobal>
#include <utility>

#include "justly/ChordsModel.hpp"

ChordCellChange::ChordCellChange(ChordsModel *chords_model_pointer_input,
                                 size_t chord_number_input,
                                 NoteChordField note_chord_field_input,
                                 QVariant old_value_input,
                                 QVariant new_value_input,
                                 QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input),
      note_chord_field(note_chord_field_input),
      old_value(std::move(old_value_input)),
      new_value(std::move(new_value_input)) {}

void ChordCellChange::undo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->set_chord_cell(chord_number, note_chord_field,
                                       old_value);
}

void ChordCellChange::redo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->set_chord_cell(chord_number, note_chord_field,
                                       new_value);
}
