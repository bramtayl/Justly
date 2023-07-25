#include "IntervalEditor.h"

#include <qnamespace.h>  // for AlignTop

#include "Interval.h"

IntervalEditor::IntervalEditor(QWidget* parent_pointer_input) : QWidget(parent_pointer_input) {
  numerator_box_pointer->setMinimum(MINIMUM_NUMERATOR);
  numerator_box_pointer->setMaximum(MAXIMUM_NUMERATOR);
  denominator_box_pointer->setMinimum(MINIMUM_DENOMINATOR);
  denominator_box_pointer->setMaximum(MAXIMUM_DENOMINATOR);
  octave_box_pointer->setMinimum(MINIMUM_OCTAVE);
  octave_box_pointer->setMaximum(MAXIMUM_OCTAVE);

  vinculum_pointer->setFrameShape(QFrame::HLine);

  column_pointer->addWidget(numerator_box_pointer);
  column_pointer->addWidget(vinculum_pointer);
  column_pointer->addWidget(denominator_box_pointer);
  fraction_widget_pointer->setLayout(column_pointer);

  row_pointer->addWidget(fraction_widget_pointer);
  row_pointer->addWidget(power_label);
  row_pointer->addWidget(octave_box_pointer);
  row_pointer->setAlignment(octave_box_pointer, Qt::AlignTop);
  setLayout(row_pointer);
  setAutoFillBackground(true);
}

auto IntervalEditor::get_interval() const -> Interval {
  return Interval(numerator_box_pointer->value(),
                  denominator_box_pointer->value(),
                  octave_box_pointer->value());
}

void IntervalEditor::set_interval(const Interval& interval) {
  numerator_box_pointer->setValue(interval.numerator);
  denominator_box_pointer->setValue(interval.denominator);
  octave_box_pointer->setValue(interval.octave);
}