#include "widgets/ControlsColumn.hpp"

#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include "widgets/CustomIntervalRow.hpp"
#include "widgets/IntervalRow.hpp"
#include "widgets/SpinBoxes.hpp"

namespace {

// 7/4, which the just scale's 9/5 minor seventh doesn't cover
const auto HARMONIC_SEVENTH_NUMERATOR = 7;

auto make_interval_row(QUndoStack& undo_stack, SwitchTable& switch_table,
                       const int halfsteps) -> IntervalRow& {
  const auto& named_ratio = get_just_scale()[halfsteps];
  return *new IntervalRow(undo_stack, switch_table, named_ratio.name,
                          Interval(named_ratio.ratio, 0));
}

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
      third_row(
          make_interval_row(undo_stack, switch_table, MAJOR_THIRD_HALFSTEPS)),
      fifth_row(
          make_interval_row(undo_stack, switch_table, PERFECT_FIFTH_HALFSTEPS)),
      seventh_row(*new IntervalRow(
          undo_stack, switch_table, "Harmonic seventh",
          Interval(Rational(HARMONIC_SEVENTH_NUMERATOR, 4), 0))),
      octave_row(*new IntervalRow(undo_stack, switch_table, "Octave",
                                  Interval(Rational(), 1))),
      custom_row(*new CustomIntervalRow(undo_stack, switch_table)) {
  auto& column_layout =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QVBoxLayout(this));
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
