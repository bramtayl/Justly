#include "commands.h"

#include <qnamespace.h>  // for DisplayRole
#include <qpointer.h>    // for QPointer
#include <qvariant.h>    // for QVariant

#include <utility>  // for move

#include "ChordsModel.h"  // for ChordsModel
#include "Editor.h"       // for Editor
#include "ShowSlider.h"   // for ShowSlider
#include "Song.h"         // for Song
#include "TreeNode.h"     // for TreeNode

class QModelIndex;

enum CommandIds {
  starting_key_change_id = 0,
  starting_volume_change_id = 1,
  starting_tempo_change_id = 2,
  starting_instrument_change_id = 3
};

// directly_set_data will error if invalid, so need to check before
SetData::SetData(Editor &editor_input, const QModelIndex &parent_index_input,
                 QVariant new_value_input, QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      editor(editor_input),
      stable_parent_index(
          editor.chords_model_pointer->get_stable_index(parent_index_input)),
      old_value(editor.chords_model_pointer->data(parent_index_input,
                                                  Qt::DisplayRole)),
      new_value(std::move(new_value_input)) {}

void SetData::redo() {
  editor.register_changed();
  editor.chords_model_pointer->directly_set_data(
      editor.chords_model_pointer->get_unstable_index(stable_parent_index),
      new_value);
}

void SetData::undo() {
  editor.chords_model_pointer->directly_set_data(
      editor.chords_model_pointer->get_unstable_index(stable_parent_index),
      old_value);
}

Remove::Remove(Editor &editor_input, int first_index_input,
               int number_of_rows_input, const QModelIndex &parent_index_input,
               QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      editor(editor_input),
      first_index(first_index_input),
      number_of_children(number_of_rows_input),
      stable_parent_index(
          editor.chords_model_pointer->get_stable_index(parent_index_input)){};

// remove_save will check for errors, so no need to check here
auto Remove::redo() -> void {
  editor.register_changed();
  editor.chords_model_pointer->remove_save(
      first_index, number_of_children,
      editor.chords_model_pointer->get_unstable_index(stable_parent_index),
      deleted_children);
}

auto Remove::undo() -> void {
  editor.chords_model_pointer->insert_children(
      first_index, deleted_children,
      editor.chords_model_pointer->get_unstable_index(stable_parent_index));
}

Insert::Insert(Editor &editor_input, int first_index_input,
               QJsonArray insertion_input,
               const QModelIndex &parent_index_input,
               QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      editor(editor_input),
      first_index(first_index_input),
      insertion(std::move(insertion_input)),
      stable_parent_index(
          editor.chords_model_pointer->get_stable_index(parent_index_input)){};

// remove_save will check for errors, so no need to check here
auto Insert::redo() -> void {
  editor.register_changed();
  editor.chords_model_pointer->insert_json_children(
      first_index, insertion,
      editor.chords_model_pointer->get_unstable_index(stable_parent_index));
}

auto Insert::undo() -> void {
  editor.chords_model_pointer->removeRows(
      first_index, static_cast<int>(insertion.size()),
      editor.chords_model_pointer->get_unstable_index(stable_parent_index));
}

InsertEmptyRows::InsertEmptyRows(Editor &editor_input, int first_index_input,
                                 int number_of_rows_input,
                                 const QModelIndex &parent_index_input,
                                 QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      editor(editor_input),
      first_index(first_index_input),
      number_of_children(number_of_rows_input),
      stable_parent_index(
          editor.chords_model_pointer->get_stable_index(parent_index_input)) {}

void InsertEmptyRows::redo() {
  editor.register_changed();
  editor.chords_model_pointer->insertRows(
      first_index, number_of_children,
      editor.chords_model_pointer->get_unstable_index(stable_parent_index));
}

void InsertEmptyRows::undo() {
  editor.chords_model_pointer->removeRows(
      first_index, number_of_children,
      editor.chords_model_pointer->get_unstable_index(stable_parent_index));
}

StartingKeyChange::StartingKeyChange(Editor &editor_input,
                                     double new_value_input)
    : editor(editor_input),
      old_value(editor_input.song.starting_key),
      new_value(new_value_input) {}

// set frequency will emit a signal to update the slider
void StartingKeyChange::redo() {
  editor.register_changed();
  if (!first_time) {
    editor.starting_key_show_slider_pointer->set_value_no_signals(new_value);
  }
  editor.song.starting_key = new_value;
  if (first_time) {
    first_time = false;
  }
}

void StartingKeyChange::undo() {
  editor.starting_key_show_slider_pointer->set_value_no_signals(old_value);
  editor.song.starting_key = old_value;
}

auto StartingKeyChange::id() const -> int { return starting_key_change_id; }

auto StartingKeyChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  new_value =
      dynamic_cast<const StartingKeyChange *>(next_command_pointer)->new_value;
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
  editor.register_changed();
  if (!first_time) {
    editor.starting_volume_show_slider_pointer->set_value_no_signals(new_value);
  }
  editor.song.starting_volume = new_value;
  if (first_time) {
    first_time = false;
  }
}

void StartingVolumeChange::undo() {
  editor.starting_volume_show_slider_pointer->set_value_no_signals(old_value);
  editor.song.starting_volume = old_value;
}

auto StartingVolumeChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  new_value = dynamic_cast<const StartingVolumeChange *>(next_command_pointer)
                  ->new_value;
  return true;
}

StartingTempoChange::StartingTempoChange(Editor &editor_input,
                                         double new_value_input)
    : editor(editor_input),
      old_value(editor_input.song.starting_tempo),
      new_value(new_value_input) {}

auto StartingTempoChange::id() const -> int { return starting_tempo_change_id; }

auto StartingTempoChange::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  new_value = dynamic_cast<const StartingTempoChange *>(next_command_pointer)
                  ->new_value;
  return true;
}

void StartingTempoChange::redo() {
  editor.register_changed();
  if (!first_time) {
    editor.starting_tempo_show_slider_pointer->set_value_no_signals(new_value);
  }
  editor.song.starting_tempo = new_value;
  if (first_time) {
    first_time = false;
  }
}

void StartingTempoChange::undo() {
  editor.starting_tempo_show_slider_pointer->set_value_no_signals(old_value);
  editor.song.starting_tempo = old_value;
}

StartingInstrumentChange::StartingInstrumentChange(
    Editor &editor_input, QString new_starting_instrument_input)
    : editor(editor_input),
      old_starting_instrument(editor_input.song.starting_instrument),
      new_starting_instrument(std::move(new_starting_instrument_input)) {}

void StartingInstrumentChange::undo() {
  editor.set_starting_instrument(old_starting_instrument, true);
}

void StartingInstrumentChange::redo() {
  editor.register_changed();
  if (first_time) {
    first_time = false;
  }
  editor.set_starting_instrument(new_starting_instrument, !first_time);
}

auto StartingInstrumentChange::id() const -> int {
  return starting_instrument_change_id;
}

auto StartingInstrumentChange::mergeWith(
    const QUndoCommand *next_command_pointer) -> bool {
  new_starting_instrument =
      dynamic_cast<const StartingInstrumentChange *>(next_command_pointer)
          ->new_starting_instrument;
  return true;
}