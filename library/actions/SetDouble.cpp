#include "actions/SetDouble.hpp"

#include <fluidsynth.h>

#include <QtWidgets/QSpinBox>

#include "actions/ChangeId.hpp"
#include "other/Song.hpp"
#include "sound/FluidSynth.hpp"

namespace {
void set_double(Song& song, FluidSynth& synth, const ChangeId control_id,
                QDoubleSpinBox& spin_box, const double set_value) {
  switch (control_id) {
    case ChangeId::gain_id:
      fluid_synth_set_gain(synth.internal_pointer,
                           static_cast<float>(set_value));
      break;
    case ChangeId::starting_key_id:
      song.starting_key = set_value;
      break;
    case ChangeId::starting_velocity_id:
      song.starting_velocity = set_value;
      break;
    case ChangeId::starting_tempo_id:
      song.starting_tempo = set_value;
      break;
    case ChangeId::replace_table_id:
      // not a spin-box control; see ReplaceTable
      Q_UNREACHABLE();
  }
  const QSignalBlocker blocker(spin_box);
  spin_box.setValue(set_value);
}
}  // namespace

auto SetDouble::id() const -> int { return static_cast<int>(control_id); }

auto SetDouble::mergeWith(const QUndoCommand* const next_command_pointer)
    -> bool {
  Q_ASSERT(next_command_pointer != nullptr);
  new_value =
      get_reference(dynamic_cast<const SetDouble*>(next_command_pointer))
          .new_value;
  return true;
}

void SetDouble::undo() {
  set_double(song, synth, control_id, spin_box, old_value);
}

void SetDouble::redo() {
  set_double(song, synth, control_id, spin_box, new_value);
}
