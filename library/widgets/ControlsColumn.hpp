#pragma once

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QWidget>

#include "cell_types/Interval.hpp"
#include "cell_types/Rational.hpp"
#include "widgets/IntervalRow.hpp"
#include "widgets/SpinBoxes.hpp"

class QUndoStack;
struct FluidSynth;
struct Song;
struct SwitchTable;

static const auto FIVE = 5;
static const auto SEVEN = 7;

struct ControlsColumn : public QWidget {
  SpinBoxes &spin_boxes;
  IntervalRow &third_row;
  IntervalRow &fifth_row;
  IntervalRow &seventh_row;
  IntervalRow &octave_row;
  QBoxLayout &column_layout = *(new QVBoxLayout(this));

  ControlsColumn(Song &song, FluidSynth &synth, QUndoStack &undo_stack,
                 SwitchTable &switch_table);
};
