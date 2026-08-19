#pragma once

#include <QtCore/QObject>
#include <QtCore/QtAssert>
#include <QtGui/QUndoCommand>
#include <QtWidgets/QDoubleSpinBox>
#include <fluidsynth.h>
#include <fluidsynth/synth.h>

#include "actions/ChangeId.hpp"
#include "other/Song.hpp"
#include "other/helpers.hpp"
#include "sound/FluidSynth.hpp"

struct SetDouble : public QUndoCommand {
  Song &song;
  FluidSynth &synth;
  QDoubleSpinBox &spin_box;
  const ChangeId control_id;
  const double old_value;
  double new_value;

  explicit SetDouble(Song &song_input, FluidSynth &synth_input,
                     QDoubleSpinBox &spin_box_input,
                     const ChangeId command_id_input,
                     const double old_value_input, const double new_value_input)
      : song(song_input), synth(synth_input), spin_box(spin_box_input),
        control_id(command_id_input), old_value(old_value_input),
        new_value(new_value_input) {}

  [[nodiscard]] auto id() const -> int override;

  [[nodiscard]] auto
  mergeWith(const QUndoCommand *const next_command_pointer) -> bool override;

  void undo() override;

  void redo() override;
};
