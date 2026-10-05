#include "widgets/SpinBoxes.hpp"

#include <fluidsynth.h>

#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>

#include "actions/ChangeId.hpp"
#include "actions/SetDouble.hpp"
#include "other/Song.hpp"
#include "rows/Note.hpp"
#include "sound/FluidSynth.hpp"

namespace {

void add_set_double(QUndoStack& undo_stack, Song& song, FluidSynth& synth,
                    QDoubleSpinBox& spin_box, const ChangeId control_id,
                    const double old_value, const double new_value) {
  undo_stack.push(new SetDouble(  // NOLINT(cppcoreguidelines-owning-memory)
      song, synth, spin_box, control_id, old_value, new_value));
}

void add_control(QFormLayout& spin_boxes_form, const QString& label,
                 QDoubleSpinBox& spin_box, const int minimum, const int maximum,
                 const QString& suffix, const double single_step = 1,
                 const int decimals = 0) {
  Q_ASSERT(suffix.isValidUtf16());
  Q_ASSERT(label.isValidUtf16());
  spin_box.setMinimum(minimum);
  spin_box.setMaximum(maximum);
  spin_box.setSuffix(suffix);
  spin_box.setSingleStep(single_step);
  spin_box.setDecimals(decimals);
  spin_boxes_form.addRow(label, &spin_box);
}

}  // namespace

void clear_and_clean(QUndoStack& undo_stack) {
  undo_stack.clear();
  undo_stack.setClean();
}

SpinBoxes::SpinBoxes(Song& song, FluidSynth& synth, QUndoStack& undo_stack)
    : gain_editor(*(new QDoubleSpinBox)),
      starting_key_editor(*(new QDoubleSpinBox)),
      starting_velocity_editor(*(new QDoubleSpinBox)),
      starting_tempo_editor(*(new QDoubleSpinBox)) {
  static const auto DEFAULT_GAIN = 5;
  static const auto GAIN_STEP = 0.1;
  static const auto MAX_GAIN = 10;
  static const auto MAX_KEY = 999;
  static const auto MAX_TEMPO = 999;

  auto& spin_boxes_form =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QFormLayout(this));
  add_control(spin_boxes_form, SpinBoxes::tr("&Gain:"), gain_editor, 0,
              MAX_GAIN, SpinBoxes::tr("/10"), GAIN_STEP, 1);
  add_control(spin_boxes_form, SpinBoxes::tr("Starting &key:"),
              starting_key_editor, 1, MAX_KEY, SpinBoxes::tr(" hz"));
  add_control(spin_boxes_form, SpinBoxes::tr("Starting &velocity:"),
              starting_velocity_editor, 1, MAX_VELOCITY, SpinBoxes::tr("/127"));
  add_control(spin_boxes_form, SpinBoxes::tr("Starting &tempo:"),
              starting_tempo_editor, 1, MAX_TEMPO, SpinBoxes::tr(" bpm"));
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  // every edit becomes an undoable SetDouble, starting from whatever
  // get_old_value reads at the time
  const auto connect_control = [this, &undo_stack, &song, &synth](
                                   QDoubleSpinBox& spin_box,
                                   const ChangeId control_id,
                                   auto get_old_value) -> void {
    QObject::connect(&spin_box, &QDoubleSpinBox::valueChanged, this,
                     [&undo_stack, &song, &synth, &spin_box, control_id,
                      get_old_value](const double new_value) -> auto {
                       add_set_double(undo_stack, song, synth, spin_box,
                                      control_id, get_old_value(), new_value);
                     });
  };
  connect_control(gain_editor, ChangeId::gain_id, [&synth]() -> double {
    return fluid_synth_get_gain(synth.internal_pointer);
  });
  connect_control(starting_key_editor, ChangeId::starting_key_id,
                  [&song]() -> double { return song.starting_key; });
  connect_control(starting_velocity_editor, ChangeId::starting_velocity_id,
                  [&song]() -> double { return song.starting_velocity; });
  connect_control(starting_tempo_editor, ChangeId::starting_tempo_id,
                  [&song]() -> double { return song.starting_tempo; });

  gain_editor.setValue(DEFAULT_GAIN);
  starting_key_editor.setValue(song.starting_key);
  starting_velocity_editor.setValue(song.starting_velocity);
  starting_tempo_editor.setValue(song.starting_tempo);

  clear_and_clean(undo_stack);
}
