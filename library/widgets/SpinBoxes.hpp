#pragma once

#include <QtWidgets/QWidget>

class QDoubleSpinBox;
class QUndoStack;
struct FluidSynth;
struct Song;

struct SpinBoxes : public QWidget {
  QDoubleSpinBox& gain_editor;
  QDoubleSpinBox& starting_key_editor;
  QDoubleSpinBox& starting_velocity_editor;
  QDoubleSpinBox& starting_tempo_editor;

  explicit SpinBoxes(Song& song, FluidSynth& synth, QUndoStack& undo_stack);
};
