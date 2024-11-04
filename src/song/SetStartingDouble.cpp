#include "song/SetStartingDouble.hpp"

#include <QSpinBox>
#include <QtGlobal>
#include <fluidsynth.h>

#include "other/other.hpp"
#include "song/ControlId.hpp"
#include "song/Player.hpp"
#include "song/Song.hpp"
#include "song/SongEditor.hpp"

void set_starting_double(SetStartingDouble &change, bool is_new) {
  auto set_value = is_new ? change.new_value : change.old_value;
  
  auto& song_editor = change.song_editor;
  
  QDoubleSpinBox *spinbox_pointer = nullptr;
  auto &song = song_editor.song;
  switch (change.command_id) {
  case gain_id:
    spinbox_pointer = &song_editor.gain_editor;
    song.gain = set_value;
    fluid_synth_set_gain(song_editor.player.synth_pointer,
                         static_cast<float>(set_value));
    break;
  case starting_key_id:
    spinbox_pointer = &song_editor.starting_key_editor;
    song.starting_key = set_value;
    break;
  case starting_velocity_id:
    spinbox_pointer = &song_editor.starting_velocity_editor;
    song.starting_velocity = set_value;
    break;
  case starting_tempo_id:
    spinbox_pointer = &song_editor.starting_tempo_editor;
    song.starting_tempo = set_value;
  }
  Q_ASSERT(spinbox_pointer != nullptr);
  auto &spin_box = *spinbox_pointer;
  spin_box.blockSignals(true);
  spin_box.setValue(set_value);
  spin_box.blockSignals(false);
}

SetStartingDouble::SetStartingDouble(SongEditor &song_editor_input,
                                     ControlId command_id_input,
                                     double new_value_input)
    : song_editor(song_editor_input), command_id(command_id_input),
      old_value(get_double(song_editor_input.song, command_id)),
      new_value(new_value_input){};

auto SetStartingDouble::id() const -> int { return starting_velocity_id; }

auto SetStartingDouble::mergeWith(const QUndoCommand *next_command_pointer)
    -> bool {
  Q_ASSERT(next_command_pointer != nullptr);

  const auto *next_velocity_change_pointer =
      dynamic_cast<const SetStartingDouble *>(next_command_pointer);

  new_value = get_const_reference(next_velocity_change_pointer).new_value;
  return true;
}

void SetStartingDouble::undo() { set_starting_double(*this, false); }

void SetStartingDouble::redo() { set_starting_double(*this, true); }
