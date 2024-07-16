#include "changes/NoteCellChange.hpp"

#include <QVariant>
#include <QtGlobal>
#include <utility>

#include "justly/ChangeId.hpp"
#include "justly/ChordsModel.hpp"

NoteCellChange::NoteCellChange(ChordsModel *chords_model_pointer_input,
                               size_t chord_number_input,
                               size_t note_number_input,
                               NoteChordField note_chord_field_input,
                               QVariant old_value_input,
                               QVariant new_value_input,
                               QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), note_number(note_number_input),
      note_chord_field(note_chord_field_input),
      old_value(std::move(old_value_input)),
      new_value(std::move(new_value_input)) {}

auto NoteCellChange::id() const -> int { return note_cell_id; }

auto NoteCellChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  Q_ASSERT(next_command_pointer != nullptr);

  const auto *next_note_cell_change_pointer =
      dynamic_cast<const NoteCellChange *>(next_command_pointer);

  Q_ASSERT(next_note_cell_change_pointer != nullptr);
  if (next_note_cell_change_pointer->chord_number == chord_number &&
      next_note_cell_change_pointer->note_number == note_number &&
      next_note_cell_change_pointer->note_chord_field == note_chord_field) {
    new_value = next_note_cell_change_pointer->new_value;
    return true;
  }
  return false;
}

void NoteCellChange::undo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->set_note_cell(chord_number, note_number,
                                      note_chord_field, old_value);
}

void NoteCellChange::redo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->set_note_cell(chord_number, note_number,
                                      note_chord_field, new_value);
}
