#include "widgets/ControlsColumn.hpp"

#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include "widgets/CustomIntervalRow.hpp"
#include "widgets/IntervalRow.hpp"
#include "widgets/SpinBoxes.hpp"

namespace {

void set_buttons_are_enabled(QPushButton& minus_button,
                             QPushButton& plus_button, const bool is_enabled) {
  minus_button.setEnabled(is_enabled);
  plus_button.setEnabled(is_enabled);
}

}  // namespace

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
                                  Interval(Rational(), 1))),
      custom_row(*new CustomIntervalRow(undo_stack, switch_table)),
      column_layout(*(new QVBoxLayout(this))) {
  column_layout.addWidget(&spin_boxes);
  column_layout.addWidget(&third_row);
  column_layout.addWidget(&fifth_row);
  column_layout.addWidget(&seventh_row);
  column_layout.addWidget(&octave_row);
  column_layout.addWidget(&custom_row);
  set_interval_rows_are_enabled(*this, false);
}

void set_interval_rows_are_enabled(ControlsColumn& controls_column,
                                   const bool is_enabled) {
  for (auto* const interval_row_pointer :
       {&controls_column.third_row, &controls_column.fifth_row,
        &controls_column.seventh_row, &controls_column.octave_row}) {
    set_buttons_are_enabled(interval_row_pointer->minus_button,
                            interval_row_pointer->plus_button, is_enabled);
  }
  set_buttons_are_enabled(controls_column.custom_row.minus_button,
                          controls_column.custom_row.plus_button, is_enabled);
}
