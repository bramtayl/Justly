#include "song/SetStartingDouble.hpp"

#include <QObject>
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

  auto &song_editor = change.song_editor;
  auto &song = song_editor.song;
  switch (change.command_id) {
  case gain_id:
    fluid_synth_set_gain(song_editor.player.synth_pointer,
                         static_cast<float>(set_value));
    break;
  case starting_key_id:
    song.starting_key = set_value;
    break;
  case starting_velocity_id:
    song.starting_velocity = set_value;
    break;
  case starting_tempo_id:
    song.starting_tempo = set_value;
  }
  auto &spin_box = change.spin_box;
  const QSignalBlocker blocker(spin_box);
  spin_box.setValue(set_value);
}

SetStartingDouble::SetStartingDouble(SongEditor &song_editor_input,
                                     QDoubleSpinBox &spin_box_input,
                                     ControlId command_id_input,
                                     double old_value_input,
                                     double new_value_input)
    : song_editor(song_editor_input), spin_box(spin_box_input),
      command_id(command_id_input),
      old_value(old_value_input),
      new_value(new_value_input){};

auto SetStartingDouble::id() const -> int { return command_id; }

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
