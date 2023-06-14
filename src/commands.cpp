#include "commands.h"

#include <qnamespace.h>  // for DisplayRole
#include <qslider.h>     // for QSlider
#include <qvariant.h>    // for QVariant

#include <algorithm>  // for max
#include <utility>    // for move

#include "Editor.h"      // for Editor
#include "ShowSlider.h"  // for ShowSlider
#include "Song.h"        // for Song

// setData_directly will error if invalid, so need to check before
CellChange::CellChange(Song &song_input, const QModelIndex &index_input,
                       QVariant new_value_input, QUndoCommand *parent_input)
    : QUndoCommand(parent_input),
      song(song_input),
      index(index_input),
      old_value(song_input.data(index, Qt::DisplayRole)),
      new_value(std::move(new_value_input)) {}

void CellChange::redo() { song.setData_directly(index, new_value); }

void CellChange::undo() { song.setData_directly(index, old_value); }

Remove::Remove(Song &song_input, int position_input, size_t rows_input,
               const QModelIndex &parent_index_input,
               QUndoCommand *parent_input)
    : QUndoCommand(parent_input),
      song(song_input),
      position(position_input),
      rows(rows_input),
      parent_index(parent_index_input){};

// remove_save will check for errors, so no need to check here
auto Remove::redo() -> void {
  song.remove_save(position, rows, parent_index, deleted_rows);
}

auto Remove::undo() -> void {
  song.insert_children(position, deleted_rows, parent_index);
}

Insert::Insert(Song &song_input, int position_input,
               std::vector<std::unique_ptr<TreeNode>> &copied,
               const QModelIndex &parent_index_input,
               QUndoCommand *parent_input)
    : QUndoCommand(parent_input),
      song(song_input),
      position(position_input),
      rows(copied.size()),
      parent_index(parent_index_input) {
  for (auto &node_pointer : copied) {
    // copy clipboard so we can paste multiple times
    // reparent too
    inserted.push_back(std::make_unique<TreeNode>(
        *(node_pointer), &(song.node_from_index(parent_index))));
  }
};

// remove_save will check for errors, so no need to check here
auto Insert::redo() -> void {
  song.insert_children(position, inserted, parent_index);
}

auto Insert::undo() -> void {
  song.remove_save(position, rows, parent_index, inserted);
}

InsertEmptyRows::InsertEmptyRows(Song &song_input, int position_input,
                                 int rows_input,
                                 const QModelIndex &parent_index_input,
                                 QUndoCommand *parent_input)
    : QUndoCommand(parent_input),
      song(song_input),
      position(position_input),
      rows(rows_input),
      parent_index(parent_index_input) {}

void InsertEmptyRows::redo() { song.insertRows(position, rows, parent_index); }

void InsertEmptyRows::undo() { song.removeRows(position, rows, parent_index); }

FrequencyChange::FrequencyChange(Editor &editor_input, int new_value_input)
    : editor(editor_input),
      old_value(editor_input.song.starting_key),
      new_value(new_value_input) {}

// set frequency will emit a signal to update the slider
void FrequencyChange::redo() {
  if (!first_time) {
    editor.frequency_slider.slider.setValue(new_value);
  }
  editor.song.starting_key = new_value;
  if (first_time) {
    first_time = false;
  }
}

void FrequencyChange::undo() {
  if (!first_time) {
    editor.frequency_slider.slider.setValue(old_value);
  }
  editor.song.starting_key = old_value;
}

VolumeChange::VolumeChange(Editor &editor_input, int new_value_input)
    : editor(editor_input),
      old_value(editor_input.song.volume_percent),
      new_value(new_value_input) {}

void VolumeChange::redo() {
  if (!first_time) {
    editor.volume_percent_slider.slider.setValue(new_value);
  }
  editor.song.volume_percent = new_value;
  if (first_time) {
    first_time = false;
  }
}

void VolumeChange::undo() {
  editor.volume_percent_slider.slider.setValue(old_value);
  editor.song.volume_percent = old_value;
}

TempoChange::TempoChange(Editor &editor_input, int new_value_input)
    : editor(editor_input),
      old_value(editor_input.song.tempo),
      new_value(new_value_input) {}

void TempoChange::redo() {
  if (!first_time) {
    editor.tempo_slider.slider.setValue(new_value);
  }
  editor.song.tempo = new_value;
  if (first_time) {
    first_time = false;
  }
}

void TempoChange::undo() {
  editor.tempo_slider.slider.setValue(old_value);
  editor.song.tempo = old_value;
}

OrchestraChange::OrchestraChange(Editor &editor, QString old_text,
                                 QString new_text)
    : editor(editor),
      old_text(std::move(old_text)),
      new_text(std::move(new_text)) {}

void OrchestraChange::undo() { editor.set_orchestra_text(old_text, true); }

void OrchestraChange::redo() {
  if (first_time) {
    first_time = false;
  }
  editor.set_orchestra_text(new_text, !first_time);
}

DefaultInstrumentChange::DefaultInstrumentChange(Editor &editor,
                                                 QString old_text,
                                                 QString new_text)
    : editor(editor),
      old_text(std::move(old_text)),
      new_text(std::move(new_text)) {}

void DefaultInstrumentChange::undo() {
  editor.set_default_instrument(old_text, true);
}

void DefaultInstrumentChange::redo() {
  if (first_time) {
    first_time = false;
  }
  editor.set_default_instrument(new_text, !first_time);
}