#include "editors/IntervalEditor.h"

#include <qboxlayout.h>  // for QHBoxLayout, QVBoxLayout
#include <qframe.h>      // for QFrame, QFrame::HLine
#include <qlabel.h>      // for QLabel
#include <qnamespace.h>  // for AlignTop
#include <qspinbox.h>    // for QSpinBox
#include <qwidget.h>     // for QWidget

#include <gsl/pointers>

#include "metatypes/Interval.h"  // for Interval, MAXIMUM_DENOMINATOR, MAX...

IntervalEditor::IntervalEditor(QWidget* parent_pointer_input)
    : QFrame(parent_pointer_input) {
  numerator_box_pointer->setMinimum(MINIMUM_NUMERATOR);
  numerator_box_pointer->setMaximum(MAXIMUM_NUMERATOR);
  denominator_box_pointer->setMinimum(MINIMUM_DENOMINATOR);
  denominator_box_pointer->setMaximum(MAXIMUM_DENOMINATOR);
  octave_box_pointer->setMinimum(MINIMUM_OCTAVE);
  octave_box_pointer->setMaximum(MAXIMUM_OCTAVE);

  auto fraction_widget_pointer = gsl::not_null(new QWidget(this));

  auto vinculum_pointer = gsl::not_null(new QFrame(fraction_widget_pointer));

  vinculum_pointer->setFrameShape(QFrame::HLine);

  auto column_pointer =
    gsl::not_null(new QVBoxLayout(fraction_widget_pointer));

  column_pointer->addWidget(numerator_box_pointer);
  column_pointer->addWidget(vinculum_pointer);
  column_pointer->addWidget(denominator_box_pointer);

  fraction_widget_pointer->setLayout(column_pointer);

  auto row_pointer = gsl::not_null(new QHBoxLayout(this));
  row_pointer->addWidget(fraction_widget_pointer);
  row_pointer->addWidget(gsl::not_null(new QLabel("× 2", this)));
  row_pointer->addWidget(octave_box_pointer);
  row_pointer->setAlignment(octave_box_pointer, Qt::AlignTop);
  setLayout(row_pointer);

  setFrameShape(QFrame::StyledPanel);

  setAutoFillBackground(true);
}

auto IntervalEditor::value() const -> Interval {
  return Interval(numerator_box_pointer->value(),
                  denominator_box_pointer->value(),
                  octave_box_pointer->value());
}

void IntervalEditor::setValue(Interval new_value) const {
  numerator_box_pointer->setValue(new_value.numerator);
  denominator_box_pointer->setValue(new_value.denominator);
  octave_box_pointer->setValue(new_value.octave);
}
