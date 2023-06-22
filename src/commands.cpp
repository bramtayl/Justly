#include "commands.h"

#include <qnamespace.h> // for DisplayRole
#include <qpointer.h>   // for QPointer
#include <qvariant.h>   // for QVariant

#include <algorithm> // for max
#include <utility>

#include "ChordsModel.h" // for ChordsModel
#include "Editor.h"      // for Editor
#include "ShowSlider.h"  // for ShowSlider
#include "Song.h"        // for Song, CellChange

enum CommandIds {
  starting_key_change_id = 0,
  starting_volume_change_id = 1,
  starting_tempo_change_id = 2
};

// setData_directly will error if invalid, so need to check before
CellChange::CellChange(ChordsModel &chords_model_input,
                       const QModelIndex &index_input, QVariant new_value_input,
                       QUndoCommand *parent_input)
    : QUndoCommand(parent_input), chords_model(chords_model_input),
      index(index_input),
      old_value(chords_model_input.data(index, Qt::DisplayRole)),
      new_value(std::move(new_value_input)) {}

void CellChange::redo() { chords_model.setData_directly(index, new_value); }

void CellChange::undo() { chords_model.setData_directly(index, old_value); }

Remove::Remove(ChordsModel &chords_model_input, int position_input,
               size_t rows_input, const QModelIndex &parent_index_input,
               QUndoCommand *parent_input)
    : QUndoCommand(parent_input), chords_model(chords_model_input),
      position(position_input), rows(rows_input),
      parent_index(parent_index_input){};

// remove_save will check for errors, so no need to check here
auto Remove::redo() -> void {
  chords_model.remove_save(position, rows, parent_index, deleted_rows);
}

auto Remove::undo() -> void {
  chords_model.insert_children(position, deleted_rows, parent_index);
}

Insert::Insert(ChordsModel &chords_model_input, int position_input,
               std::vector<std::unique_ptr<TreeNode>> &copied,
               const QModelIndex &parent_index_input,
               QUndoCommand *parent_input)
    : QUndoCommand(parent_input), chords_model(chords_model_input),
      position(position_input), rows(copied.size()),
      parent_index(parent_index_input) {
  for (auto &node_pointer : copied) {
    // copy clipboard so we can paste multiple times
    // reparent too
    inserted.push_back(std::make_unique<TreeNode>(
        *(node_pointer), &(chords_model.node_from_index(parent_index))));
  }
};

// remove_save will check for errors, so no need to check here
auto Insert::redo() -> void {
  chords_model.insert_children(position, inserted, parent_index);
}

auto Insert::undo() -> void {
  chords_model.remove_save(position, rows, parent_index, inserted);
}

InsertEmptyRows::InsertEmptyRows(ChordsModel &chords_model_input,
                                 int position_input, int rows_input,
                                 const QModelIndex &parent_index_input,
                                 QUndoCommand *parent_input)
    : QUndoCommand(parent_input), chords_model(chords_model_input),
      position(position_input), rows(rows_input),
      parent_index(parent_index_input) {}

void InsertEmptyRows::redo() {
  chords_model.insertRows(position, rows, parent_index);
}

void InsertEmptyRows::undo() {
  chords_model.removeRows(position, rows, parent_index);
}

StartingKeyChange::StartingKeyChange(Editor &editor_input,
                                     double new_value_input)
    : editor(editor_input), old_value(editor_input.song.starting_key),
      new_value(new_value_input) {}

// set frequency will emit a signal to update the slider
void StartingKeyChange::redo() {
  if (!first_time) {
    editor.starting_key_slider_pointer->set_value_override(new_value);
  }
  editor.song.starting_key = new_value;
  if (first_time) {
    first_time = false;
  }
}

void StartingKeyChange::undo() {
  editor.starting_key_slider_pointer->set_value_override(old_value);
  editor.song.starting_key = old_value;
}

auto StartingKeyChange::id() const -> int { return starting_key_change_id; }

auto StartingKeyChange::mergeWith(const QUndoCommand *other) -> bool {
  new_value = dynamic_cast<const StartingKeyChange *>(other)->new_value;
  return true;
}

StartingVolumeChange::StartingVolumeChange(Editor &editor_input,
                                           double new_value_input)
    : editor(editor_input), old_value(editor_input.song.starting_volume),
      new_value(new_value_input) {}

auto StartingVolumeChange::id() const -> int {
  return starting_volume_change_id;
}

void StartingVolumeChange::redo() {
  if (!first_time) {
    editor.starting_volume_slider_pointer->set_value_override(new_value);
  }
  editor.song.starting_volume = new_value;
  if (first_time) {
    first_time = false;
  }
}

void StartingVolumeChange::undo() {
  editor.starting_volume_slider_pointer->set_value_override(old_value);
  editor.song.starting_volume = old_value;
}

auto StartingVolumeChange::mergeWith(const QUndoCommand *other) -> bool {
  new_value = dynamic_cast<const StartingVolumeChange *>(other)->new_value;
  return true;
}

StartingTempoChange::StartingTempoChange(Editor &editor_input,
                                         double new_value_input)
    : editor(editor_input), old_value(editor_input.song.starting_tempo),
      new_value(new_value_input) {}

auto StartingTempoChange::id() const -> int { return starting_tempo_change_id; }

auto StartingTempoChange::mergeWith(const QUndoCommand *other) -> bool {
  new_value = dynamic_cast<const StartingTempoChange *>(other)->new_value;
  return true;
}

void StartingTempoChange::redo() {
  if (!first_time) {
    editor.starting_tempo_slider_pointer->set_value_override(new_value);
  }
  editor.song.starting_tempo = new_value;
  if (first_time) {
    first_time = false;
  }
}

void StartingTempoChange::undo() {
  editor.starting_tempo_slider_pointer->set_value_override(old_value);
  editor.song.starting_tempo = old_value;
}

OrchestraChange::OrchestraChange(Editor &editor, QString old_text,
                                 QString new_text,
                                 QString old_starting_instrument,
                                 QString new_starting_instrument)
    : editor(editor), old_text(std::move(old_text)),
      new_text(std::move(new_text)),
      old_starting_instrument(std::move(old_starting_instrument)),
      new_starting_instrument(std::move(new_starting_instrument)) {}

void OrchestraChange::undo() {
  editor.set_orchestra_text(old_text, old_starting_instrument, true);
}

void OrchestraChange::redo() {
  if (first_time) {
    first_time = false;
  }
  editor.set_orchestra_text(new_text, new_starting_instrument, !first_time);
}

StartingInstrumentChange::StartingInstrumentChange(Editor &editor,
                                                   QString old_text,
                                                   QString new_text)
    : editor(editor), old_text(std::move(old_text)),
      new_text(std::move(new_text)) {}

void StartingInstrumentChange::undo() {
  editor.set_starting_instrument(old_text, true);
}

void StartingInstrumentChange::redo() {
  if (first_time) {
    first_time = false;
  }
  editor.set_starting_instrument(new_text, !first_time);
}
