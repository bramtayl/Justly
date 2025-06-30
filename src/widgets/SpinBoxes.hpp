#pragma once

#include <QtWidgets/QFormLayout>

#include "actions/SetDouble.hpp"

static const auto DEFAULT_GAIN = 5;
static const auto GAIN_STEP = 0.1;
static const auto MAX_GAIN = 10;
static const auto MAX_KEY = 999;
static const auto MAX_TEMPO = 999;

static void add_set_double(QUndoStack &undo_stack, Song &song,
                           FluidSynth &synth, QDoubleSpinBox &spin_box,
                           const ChangeId control_id, const double old_value,
                           const double new_value) {
  undo_stack.push(new SetDouble( // NOLINT(cppcoreguidelines-owning-memory)
      song, synth, spin_box, control_id, old_value, new_value));
}

static void add_control(QFormLayout &spin_boxes_form, const QString &label,
                        QDoubleSpinBox &spin_box, const int minimum,
                        const int maximum, const QString &suffix,
                        const double single_step = 1, const int decimals = 0) {
  Q_ASSERT(suffix.isValidUtf16());
  Q_ASSERT(label.isValidUtf16());
  spin_box.setMinimum(minimum);
  spin_box.setMaximum(maximum);
  spin_box.setSuffix(suffix);
  spin_box.setSingleStep(single_step);
  spin_box.setDecimals(decimals);
  spin_boxes_form.addRow(label, &spin_box);
}

static inline void clear_and_clean(QUndoStack &undo_stack) {
  undo_stack.clear();
  undo_stack.setClean();
}

struct SpinBoxes : public QWidget {
  QDoubleSpinBox &gain_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_key_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_velocity_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_tempo_editor = *(new QDoubleSpinBox);
  QFormLayout &spin_boxes_form = *(new QFormLayout(this));

  explicit SpinBoxes(Song &song, FluidSynth &synth, QUndoStack &undo_stack) {
    auto &gain_editor_ref = this->gain_editor;
    auto &starting_key_editor_ref = this->starting_key_editor;
    auto &starting_velocity_editor_ref = this->starting_velocity_editor;
    auto &starting_tempo_editor_ref = this->starting_tempo_editor;

    add_control(spin_boxes_form, SpinBoxes::tr("&Gain:"), gain_editor, 0,
                MAX_GAIN, SpinBoxes::tr("/10"), GAIN_STEP, 1);
    add_control(spin_boxes_form, SpinBoxes::tr("Starting &key:"),
                starting_key_editor, 1, MAX_KEY, SpinBoxes::tr(" hz"));
    add_control(spin_boxes_form, SpinBoxes::tr("Starting &velocity:"),
                starting_velocity_editor, 1, MAX_VELOCITY,
                SpinBoxes::tr("/127"));
    add_control(spin_boxes_form, SpinBoxes::tr("Starting &tempo:"),
                starting_tempo_editor, 1, MAX_TEMPO, SpinBoxes::tr(" bpm"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QObject::connect(
        &gain_editor, &QDoubleSpinBox::valueChanged, this,
        [&undo_stack, &song, &synth, &gain_editor_ref](double new_value) {
          add_set_double(undo_stack, song, synth, gain_editor_ref, gain_id,
                         fluid_synth_get_gain(synth.internal_pointer), new_value);
        });
    QObject::connect(&starting_key_editor, &QDoubleSpinBox::valueChanged, this,
                     [&undo_stack, &song, &synth,
                      &starting_key_editor_ref](double new_value) {
                       add_set_double(undo_stack, song, synth,
                                      starting_key_editor_ref, starting_key_id,
                                      song.starting_key, new_value);
                     });
    QObject::connect(
        &starting_velocity_editor, &QDoubleSpinBox::valueChanged, this,
        [&undo_stack, &song, &synth,
         &starting_velocity_editor_ref](double new_value) {
          add_set_double(undo_stack, song, synth, starting_velocity_editor_ref,
                         starting_velocity_id, song.starting_velocity,
                         new_value);
        });
    QObject::connect(
        &starting_tempo_editor, &QDoubleSpinBox::valueChanged, this,
        [&undo_stack, &song, &synth,
         &starting_tempo_editor_ref](double new_value) {
          add_set_double(undo_stack, song, synth, starting_tempo_editor_ref,
                         starting_tempo_id, song.starting_tempo, new_value);
        });

    gain_editor.setValue(DEFAULT_GAIN);
    starting_key_editor.setValue(song.starting_key);
    starting_velocity_editor.setValue(song.starting_velocity);
    starting_tempo_editor.setValue(song.starting_tempo);

    clear_and_clean(undo_stack);
  }
};
