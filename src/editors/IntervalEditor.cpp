#include "src/editors/IntervalEditor.h"

#include <qboxlayout.h>  // for QHBoxLayout, QVBoxLayout
#include <qframe.h>      // for QFrame, QFrame::HLine
#include <qlabel.h>      // for QLabel
#include <qnamespace.h>  // for AlignTop
#include <qspinbox.h>    // for QSpinBox
#include <qwidget.h>     // for QWidget

#include <gsl/pointers>

#include "justly/metatypes/Interval.h"  // for Interval, MAXIMUM_DENOMINATOR, MAX...

const auto SMALL_SPACING = 5;

IntervalEditor::IntervalEditor(QWidget* parent_pointer_input)
    : QFrame(parent_pointer_input) {
  setFrameStyle(QFrame::StyledPanel);
  numerator_box_pointer->setMinimum(MINIMUM_NUMERATOR);
  numerator_box_pointer->setMaximum(MAXIMUM_NUMERATOR);
  denominator_box_pointer->setMinimum(MINIMUM_DENOMINATOR);
  denominator_box_pointer->setMaximum(MAXIMUM_DENOMINATOR);
  octave_box_pointer->setMinimum(MINIMUM_OCTAVE);
  octave_box_pointer->setMaximum(MAXIMUM_OCTAVE);

  auto row_pointer = gsl::not_null(new QHBoxLayout(this));
  row_pointer->addWidget(numerator_box_pointer);
  row_pointer->addWidget(gsl::not_null(new QLabel("/", this)));
  row_pointer->addWidget(denominator_box_pointer);
  row_pointer->addWidget(gsl::not_null(new QLabel("o", this)));
  row_pointer->addWidget(octave_box_pointer);
  row_pointer->setAlignment(octave_box_pointer, Qt::AlignCenter);
  setLayout(row_pointer);

  row_pointer->setContentsMargins(SMALL_SPACING, SMALL_SPACING, SMALL_SPACING,
                                   SMALL_SPACING);

  setAutoFillBackground(true);

  setFixedSize(sizeHint());
}

auto IntervalEditor::value() const -> Interval {
  return Interval(numerator_box_pointer->value(),
                  denominator_box_pointer->value(),
                  octave_box_pointer->value());
}

void IntervalEditor::set_interval(Interval new_value) const {
  numerator_box_pointer->setValue(new_value.numerator);
  denominator_box_pointer->setValue(new_value.denominator);
  octave_box_pointer->setValue(new_value.octave);
}
