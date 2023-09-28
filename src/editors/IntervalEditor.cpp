#include "editors/IntervalEditor.h"

#include <qboxlayout.h>  // for QHBoxLayout, QVBoxLayout
#include <qframe.h>      // for QFrame, QFrame::HLine
#include <qlabel.h>      // for QLabel
#include <qnamespace.h>     // for AlignTop
#include <qspinbox.h>    // for QSpinBox
#include <qwidget.h>     // for QWidget

#include "metatypes/Interval.h"  // for Interval, MAXIMUM_DENOMINATOR, MAX...

IntervalEditor::IntervalEditor(QWidget *parent_pointer_input)
    : QWidget(parent_pointer_input) {
  numerator_box_pointer->setMinimum(MINIMUM_NUMERATOR);
  numerator_box_pointer->setMaximum(MAXIMUM_NUMERATOR);
  denominator_box_pointer->setMinimum(MINIMUM_DENOMINATOR);
  denominator_box_pointer->setMaximum(MAXIMUM_DENOMINATOR);
  octave_box_pointer->setMinimum(MINIMUM_OCTAVE);
  octave_box_pointer->setMaximum(MAXIMUM_OCTAVE);

  gsl::not_null<QFrame*> vinculum_pointer =
    std::make_unique<QFrame>(fraction_widget_pointer).release();

  vinculum_pointer->setFrameShape(QFrame::HLine);

  QVBoxLayout* column_pointer =
    std::make_unique<QVBoxLayout>(fraction_widget_pointer).release();

  column_pointer->addWidget(numerator_box_pointer);
  column_pointer->addWidget(vinculum_pointer);
  column_pointer->addWidget(denominator_box_pointer);
  
  fraction_widget_pointer->setLayout(column_pointer);

  QHBoxLayout* row_pointer =
    std::make_unique<QHBoxLayout>(this).release();
  row_pointer->addWidget(fraction_widget_pointer);
  row_pointer->addWidget(std::make_unique<QLabel>("× 2", this).release());
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

void IntervalEditor::set_interval(const Interval &interval) const {
  numerator_box_pointer->setValue(interval.numerator);
  denominator_box_pointer->setValue(interval.denominator);
  octave_box_pointer->setValue(interval.octave);
}
