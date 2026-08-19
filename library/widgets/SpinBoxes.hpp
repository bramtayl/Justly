#pragma once

#include <QtWidgets/QFormLayout>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>

class QUndoStack;
struct FluidSynth;
struct Song;

void clear_and_clean(QUndoStack &undo_stack);

struct SpinBoxes : public QWidget {
  QDoubleSpinBox &gain_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_key_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_velocity_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_tempo_editor = *(new QDoubleSpinBox);
  QFormLayout &spin_boxes_form = *(new QFormLayout(this));

  explicit SpinBoxes(Song &song, FluidSynth &synth, QUndoStack &undo_stack);
};
