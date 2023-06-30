#include "commands.h"

#include <qnamespace.h>  // for DisplayRole
#include <qpointer.h>    // for QPointer
#include <qvariant.h>    // for QVariant

#include <utility>

#include "ChordsModel.h"  // for ChordsModel
#include "Editor.h"       // for Editor
#include "ShowSlider.h"   // for ShowSlider
#include "Song.h"         // for Song, CellChange

class QModelIndex;

enum CommandIds {
  starting_key_change_id = 0,
  starting_volume_change_id = 1,
  starting_tempo_change_id = 2,
  starting_instrument_change_id = 3
};

// setData_irreversible will error if invalid, so need to check before
CellChange::CellChange(ChordsModel &chords_model_input,
                       const QModelIndex &index_input, QVariant new_value_input,
                       QUndoCommand *parent_input)
    : QUndoCommand(parent_input),
      chords_model(chords_model_input),
      stable_index(chords_model_input.get_stable_index(index_input)),
      old_value(chords_model_input.data(index_input, Qt::DisplayRole)),
      new_value(std::move(new_value_input)) {}

void CellChange::redo() {
  chords_model.setData_irreversible(
      chords_model.get_unstable_index(stable_index), new_value);
}

void CellChange::undo() {
  chords_model.setData_irreversible(
      chords_model.get_unstable_index(stable_index), old_value);
}

Remove::Remove(ChordsModel &chords_model_input, int position_input,
               int rows_input, const QModelIndex &parent_index_input,
               QUndoCommand *parent_input)
    : QUndoCommand(parent_input),
      chords_model(chords_model_input),
      position(position_input),
      rows(rows_input),
      stable_parent_index(
          chords_model_input.get_stable_index(parent_index_input)){};

// remove_save will check for errors, so no need to check here
auto Remove::redo() -> void {
  chords_model.remove_save(position, rows,
                           chords_model.get_unstable_index(stable_parent_index),
                           deleted_rows);
}

auto Remove::undo() -> void {
  chords_model.insert_children(
      position, deleted_rows,
      chords_model.get_unstable_index(stable_parent_index));
}

Insert::Insert(ChordsModel &chords_model_input, int position_input,
               std::vector<std::unique_ptr<TreeNode>> &copied,
               const QModelIndex &parent_index_input,
               QUndoCommand *parent_input)
    : QUndoCommand(parent_input),
      chords_model(chords_model_input),
      position(position_input),
      rows(static_cast<int>(copied.size())),
      stable_parent_index(
          chords_model_input.get_stable_index(parent_index_input)) {
  for (auto &node_pointer : copied) {
    // copy clipboard so we can paste multiple times
    // reparent too
    inserted.push_back(std::make_unique<TreeNode>(
        *(node_pointer), &(chords_model.node_from_index(parent_index_input))));
  }
};

// remove_save will check for errors, so no need to check here
auto Insert::redo() -> void {
  chords_model.insert_children(
      position, inserted, chords_model.get_unstable_index(stable_parent_index));
}

auto Insert::undo() -> void {
  chords_model.remove_save(position, rows,
                           chords_model.get_unstable_index(stable_parent_index),
                           inserted);
}

InsertEmptyRows::InsertEmptyRows(ChordsModel &chords_model_input,
                                 int position_input, int rows_input,
                                 const QModelIndex &parent_index_input,
                                 QUndoCommand *parent_input)
    : QUndoCommand(parent_input),
      chords_model(chords_model_input),
      position(position_input),
      rows(rows_input),
      stable_parent_index(chords_model.get_stable_index(parent_index_input)) {}

void InsertEmptyRows::redo() {
  chords_model.insertRows(position, rows,
                          chords_model.get_unstable_index(stable_parent_index));
}

void InsertEmptyRows::undo() {
  chords_model.removeRows(position, rows,
                          chords_model.get_unstable_index(stable_parent_index));
}

StartingKeyChange::StartingKeyChange(Editor &editor_input,
                                     double new_value_input)
    : editor(editor_input),
      old_value(editor_input.song.starting_key),
      new_value(new_value_input) {}

// set frequency will emit a signal to update the slider
void StartingKeyChange::redo() {
  if (!first_time) {
    editor.starting_key_show_slider_pointer->set_value_override(new_value);
  }
  editor.song.starting_key = new_value;
  if (first_time) {
    first_time = false;
  }
}

void StartingKeyChange::undo() {
  editor.starting_key_show_slider_pointer->set_value_override(old_value);
  editor.song.starting_key = old_value;
}

auto StartingKeyChange::id() const -> int { return starting_key_change_id; }

auto StartingKeyChange::mergeWith(const QUndoCommand *other) -> bool {
  new_value = dynamic_cast<const StartingKeyChange *>(other)->new_value;
  return true;
}

StartingVolumeChange::StartingVolumeChange(Editor &editor_input,
                                           double new_value_input)
    : editor(editor_input),
      old_value(editor_input.song.starting_volume),
      new_value(new_value_input) {}

auto StartingVolumeChange::id() const -> int {
  return starting_volume_change_id;
}

void StartingVolumeChange::redo() {
  if (!first_time) {
    editor.starting_volume_show_slider_pointer->set_value_override(new_value);
  }
  editor.song.starting_volume = new_value;
  if (first_time) {
    first_time = false;
  }
}

void StartingVolumeChange::undo() {
  editor.starting_volume_show_slider_pointer->set_value_override(old_value);
  editor.song.starting_volume = old_value;
}

auto StartingVolumeChange::mergeWith(const QUndoCommand *other) -> bool {
  new_value = dynamic_cast<const StartingVolumeChange *>(other)->new_value;
  return true;
}

StartingTempoChange::StartingTempoChange(Editor &editor_input,
                                         double new_value_input)
    : editor(editor_input),
      old_value(editor_input.song.starting_tempo),
      new_value(new_value_input) {}

auto StartingTempoChange::id() const -> int { return starting_tempo_change_id; }

auto StartingTempoChange::mergeWith(const QUndoCommand *other) -> bool {
  new_value = dynamic_cast<const StartingTempoChange *>(other)->new_value;
  return true;
}

void StartingTempoChange::redo() {
  if (!first_time) {
    editor.starting_tempo_show_slider_pointer->set_value_override(new_value);
  }
  editor.song.starting_tempo = new_value;
  if (first_time) {
    first_time = false;
  }
}

void StartingTempoChange::undo() {
  editor.starting_tempo_show_slider_pointer->set_value_override(old_value);
  editor.song.starting_tempo = old_value;
}

StartingInstrumentChange::StartingInstrumentChange(
    Editor &editor, QString new_starting_instrument_input)
    : editor(editor),
      old_starting_instrument(editor.song.starting_instrument),
      new_starting_instrument(std::move(new_starting_instrument_input)) {}

void StartingInstrumentChange::undo() {
  editor.set_starting_instrument(old_starting_instrument, true);
}

void StartingInstrumentChange::redo() {
  if (first_time) {
    first_time = false;
  }
  editor.set_starting_instrument(new_starting_instrument, !first_time);
}

auto StartingInstrumentChange::id() const -> int {
  return starting_instrument_change_id;
}

auto StartingInstrumentChange::mergeWith(const QUndoCommand *other) -> bool {
  new_starting_instrument =
      dynamic_cast<const StartingInstrumentChange *>(other)
          ->new_starting_instrument;
  return true;
}