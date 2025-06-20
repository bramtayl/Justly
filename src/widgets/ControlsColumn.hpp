#pragma once

#include "widgets/IntervalRow.hpp"
#include "widgets/SpinBoxes.hpp"
#include "widgets/SwitchColumn.hpp"

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
                 SwitchColumn &switch_column)
      : spin_boxes(*new SpinBoxes(song, synth, undo_stack)),
        third_row(*new IntervalRow(undo_stack, switch_column, "Major third",
                                   Interval(Rational(FIVE, 4), 0))),
        fifth_row(*new IntervalRow(undo_stack, switch_column, "Perfect fifth",
                                   Interval(Rational(3, 2), 0))),
        seventh_row(*new IntervalRow(undo_stack, switch_column,
                                     "Harmonic seventh",
                                     Interval(Rational(SEVEN, 4), 0))),
        octave_row(*new IntervalRow(undo_stack, switch_column, "Octave",
                                    Interval(Rational(), 1))) {
    column_layout.addWidget(&spin_boxes);
    column_layout.addWidget(&third_row);
    column_layout.addWidget(&fifth_row);
    column_layout.addWidget(&seventh_row);
    column_layout.addWidget(&octave_row);
    set_interval_rows_enabled(third_row, fifth_row, seventh_row, octave_row,
                              false);
  }
};
