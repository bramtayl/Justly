#include "changes/NoteCellChange.hpp"

#include <QVariant>
#include <QtGlobal>
#include <utility>

#include "justly/ChangeId.hpp"
#include "justly/ChordsModel.hpp"

NoteCellChange::NoteCellChange(ChordsModel *chords_model_pointer_input,
                               size_t child_number_input,
                               NoteChordField note_chord_field_input,
                               size_t parent_number_input,
                               QVariant old_value_input,
                               QVariant new_value_input,
                               QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      child_number(child_number_input),
      note_chord_field(note_chord_field_input),
      parent_number(parent_number_input), old_value(std::move(old_value_input)),
      new_value(std::move(new_value_input)) {}

auto NoteCellChange::id() const -> int { return cell_id; }

auto NoteCellChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  Q_ASSERT(next_command_pointer != nullptr);

  const auto *next_cell_change_pointer =
      dynamic_cast<const NoteCellChange *>(next_command_pointer);

  Q_ASSERT(next_cell_change_pointer != nullptr);
  if (next_cell_change_pointer->parent_number == parent_number &&
      next_cell_change_pointer->child_number == child_number &&
      next_cell_change_pointer->note_chord_field == note_chord_field) {
    new_value = next_cell_change_pointer->new_value;
    return true;
  }
  return false;
}

void NoteCellChange::undo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->set_note_cell(child_number, note_chord_field, parent_number, old_value);
}

void NoteCellChange::redo() {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->set_note_cell(child_number, note_chord_field, parent_number, new_value);
}
