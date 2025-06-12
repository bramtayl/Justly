#pragma once

#include <QtGui/QUndoCommand>
#include <QtWidgets/QDoubleSpinBox>

#include "actions/ChangeId.hpp"
#include "other/Song.hpp"

static void set_double(Song &song, fluid_synth_t &synth,
                       const ChangeId control_id, QDoubleSpinBox &spin_box,
                       const double set_value) {
  switch (control_id) {
  case gain_id:
    fluid_synth_set_gain(&synth, static_cast<float>(set_value));
    break;
  case starting_key_id:
    song.starting_key = set_value;
    break;
  case starting_velocity_id:
    song.starting_velocity = set_value;
    break;
  case starting_tempo_id:
    song.starting_tempo = set_value;
    break;
  default:
    Q_ASSERT(false);
    break;
  }
  const QSignalBlocker blocker(spin_box);
  spin_box.setValue(set_value);
}

struct SetDouble : public QUndoCommand {
  Song &song;
  fluid_synth_t &synth;
  QDoubleSpinBox &spin_box;
  const ChangeId control_id;
  const double old_value;
  double new_value;

  explicit SetDouble(Song &song_input, fluid_synth_t &synth_input,
                     QDoubleSpinBox &spin_box_input,
                     const ChangeId command_id_input,
                     const double old_value_input, const double new_value_input)
      : song(song_input), synth(synth_input), spin_box(spin_box_input),
        control_id(command_id_input), old_value(old_value_input),
        new_value(new_value_input) {}

  [[nodiscard]] auto id() const -> int override { return control_id; }

  [[nodiscard]] auto
  mergeWith(const QUndoCommand *const next_command_pointer) -> bool override {
    Q_ASSERT(next_command_pointer != nullptr);
    new_value =
        get_reference(dynamic_cast<const SetDouble *>(next_command_pointer))
            .new_value;
    return true;
  }

  void undo() override {
    set_double(song, synth, control_id, spin_box, old_value);
  }

  void redo() override {
    set_double(song, synth, control_id, spin_box, new_value);
  }
};
