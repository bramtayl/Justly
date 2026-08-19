#include "widgets/ControlsColumn.hpp"

#include "cell_types/Interval.hpp"
#include "cell_types/Rational.hpp"
#include "widgets/IntervalRow.hpp"
#include "widgets/SpinBoxes.hpp"

ControlsColumn::ControlsColumn(Song& song, FluidSynth& synth,
                               QUndoStack& undo_stack,
                               SwitchTable& switch_table)
    : spin_boxes(*new SpinBoxes(song, synth, undo_stack)),
      third_row(*new IntervalRow(undo_stack, switch_table, "Major third",
                                 Interval(Rational(FIVE, 4), 0))),
      fifth_row(*new IntervalRow(undo_stack, switch_table, "Perfect fifth",
                                 Interval(Rational(3, 2), 0))),
      seventh_row(*new IntervalRow(undo_stack, switch_table, "Harmonic seventh",
                                   Interval(Rational(SEVEN, 4), 0))),
      octave_row(*new IntervalRow(undo_stack, switch_table, "Octave",
                                  Interval(Rational(), 1))) {
  column_layout.addWidget(&spin_boxes);
  column_layout.addWidget(&third_row);
  column_layout.addWidget(&fifth_row);
  column_layout.addWidget(&seventh_row);
  column_layout.addWidget(&octave_row);
  set_interval_rows_is_enabled(third_row, fifth_row, seventh_row, octave_row,
                               false);
}
