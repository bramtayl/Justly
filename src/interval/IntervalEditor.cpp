#include "interval/IntervalEditor.hpp"

#include <QBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QSpinBox>
#include <memory>

#include "interval/Interval.hpp"
#include "other/bounds.hpp"

class QWidget;

IntervalEditor::IntervalEditor(QWidget *parent_pointer_input)
    : QFrame(parent_pointer_input), numerator_box_pointer(new QSpinBox(this)),
      denominator_box_pointer(new QSpinBox(this)),
      octave_box_pointer(new QSpinBox(this)) {
  setFrameStyle(QFrame::StyledPanel);
  setAutoFillBackground(true);

  numerator_box_pointer->setMinimum(1);
  numerator_box_pointer->setMaximum(MAX_INTERVAL_NUMERATOR);

  denominator_box_pointer->setMinimum(1);
  denominator_box_pointer->setMaximum(MAX_INTERVAL_DENOMINATOR);

  octave_box_pointer->setMinimum(MIN_OCTAVE);
  octave_box_pointer->setMaximum(MAX_OCTAVE);

  auto *row_pointer = std::make_unique<QHBoxLayout>(this).release();
  row_pointer->addWidget(numerator_box_pointer);
  row_pointer->addWidget(std::make_unique<QLabel>("/", this).release());
  row_pointer->addWidget(denominator_box_pointer);
  row_pointer->addWidget(std::make_unique<QLabel>("o", this).release());
  row_pointer->addWidget(octave_box_pointer);

  setLayout(row_pointer);

  setMinimumSize(sizeHint());
}

auto IntervalEditor::value() const -> Interval {
  return Interval({numerator_box_pointer->value(),
                   denominator_box_pointer->value(),
                   octave_box_pointer->value()});
}

void IntervalEditor::setValue(Interval new_value) const {
  numerator_box_pointer->setValue(new_value.numerator);
  denominator_box_pointer->setValue(new_value.denominator);
  octave_box_pointer->setValue(new_value.octave);
}
