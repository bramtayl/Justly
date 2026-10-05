#pragma once

#include <QtWidgets/QWidget>

class QUndoStack;
struct CustomIntervalRow;
struct FluidSynth;
struct Song;
struct SwitchTable;
struct IntervalRow;
struct SpinBoxes;

struct ControlsColumn : public QWidget {
  SpinBoxes& spin_boxes;
  IntervalRow& third_row;
  IntervalRow& fifth_row;
  IntervalRow& seventh_row;
  IntervalRow& octave_row;
  CustomIntervalRow& custom_row;

  ControlsColumn(Song& song, FluidSynth& synth, QUndoStack& undo_stack,
                 SwitchTable& switch_table);
};

void set_interval_rows_are_enabled(ControlsColumn& controls_column,
                                   bool is_enabled);
