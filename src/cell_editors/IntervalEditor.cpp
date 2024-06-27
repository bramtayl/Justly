#include "cell_editors/IntervalEditor.hpp"

#include <qboxlayout.h>  // for QHBoxLayout
#include <qframe.h>      // for QFrame, QFrame::StyledPanel
#include <qlabel.h>      // for QLabel
#include <qnamespace.h>  // for AlignCenter
#include <qspinbox.h>    // for QSpinBox

#include <memory>  // for make_unique, __unique_ptr_t

#include "justly/Interval.hpp"          // for Interval
#include "justly/public_constants.hpp"  // for MAX_DENOMINATOR, MAX_NUMERATOR

const auto SMALL_SPACING = 1;

IntervalEditor::IntervalEditor(QWidget* parent_pointer_input)
    : QFrame(parent_pointer_input) {

  setFrameStyle(QFrame::StyledPanel);

  Q_ASSERT(numerator_box_pointer != nullptr);
  numerator_box_pointer->setMinimum(1);
  numerator_box_pointer->setMaximum(MAX_NUMERATOR);

  Q_ASSERT(denominator_box_pointer != nullptr);
  denominator_box_pointer->setMinimum(1);
  denominator_box_pointer->setMaximum(MAX_DENOMINATOR);

  Q_ASSERT(octave_box_pointer != nullptr);
  octave_box_pointer->setMinimum(MIN_OCTAVE);
  octave_box_pointer->setMaximum(MAX_OCTAVE);

  auto* row_pointer = std::make_unique<QHBoxLayout>(this).release();
  row_pointer->addWidget(numerator_box_pointer);
  row_pointer->addWidget(std::make_unique<QLabel>("/", this).release());
  row_pointer->addWidget(denominator_box_pointer);
  row_pointer->addWidget(std::make_unique<QLabel>("o", this).release());
  row_pointer->addWidget(octave_box_pointer);
  row_pointer->setAlignment(octave_box_pointer, Qt::AlignCenter);
  setLayout(row_pointer);

  row_pointer->setContentsMargins(SMALL_SPACING, SMALL_SPACING, SMALL_SPACING,
                                  SMALL_SPACING);

  setAutoFillBackground(true);

  setFixedSize(sizeHint());
}

auto IntervalEditor::get_interval() const -> Interval {
  Q_ASSERT(numerator_box_pointer != nullptr);
  Q_ASSERT(denominator_box_pointer != nullptr);
  Q_ASSERT(octave_box_pointer != nullptr);
  return Interval(numerator_box_pointer->value(),
                  denominator_box_pointer->value(),
                  octave_box_pointer->value());
}

void IntervalEditor::set_interval(Interval new_value) const {
  Q_ASSERT(numerator_box_pointer != nullptr);
  Q_ASSERT(denominator_box_pointer != nullptr);
  Q_ASSERT(octave_box_pointer != nullptr);

  numerator_box_pointer->setValue(new_value.numerator);
  denominator_box_pointer->setValue(new_value.denominator);
  octave_box_pointer->setValue(new_value.octave);
}
