#include "cell_editors/RationalEditor.hpp"

#include <qassert.h>     // for Q_ASSERT
#include <qboxlayout.h>  // for QHBoxLayout
#include <qframe.h>      // for QFrame, QFrame::StyledPanel
#include <qlabel.h>      // for QLabel
#include <qspinbox.h>    // for QSpinBox

#include <memory>  // for make_unique, __unique_ptr_t

#include "justly/Rational.hpp"          // for Rational
#include "other/private_constants.hpp"  // for MAX_DENOMINATOR, MAX_NUMERATOR

const auto SMALL_SPACING = 1;

RationalEditor::RationalEditor(QWidget* parent_pointer_input)
    : QFrame(parent_pointer_input) {
  setFrameStyle(QFrame::StyledPanel);

  Q_ASSERT(numerator_box_pointer != nullptr);
  numerator_box_pointer->setMinimum(1);
  numerator_box_pointer->setMaximum(MAX_NUMERATOR);

  Q_ASSERT(denominator_box_pointer != nullptr);
  denominator_box_pointer->setMinimum(1);
  denominator_box_pointer->setMaximum(MAX_DENOMINATOR);

  auto* row_pointer = std::make_unique<QHBoxLayout>(this).release();
  row_pointer->addWidget(numerator_box_pointer);
  row_pointer->addWidget(std::make_unique<QLabel>("/", this).release());
  row_pointer->addWidget(denominator_box_pointer);
  row_pointer->setContentsMargins(SMALL_SPACING, SMALL_SPACING, SMALL_SPACING,
                                  SMALL_SPACING);
  setLayout(row_pointer);

  setAutoFillBackground(true);
  setFixedSize(sizeHint());
}

auto RationalEditor::get_rational() const -> Rational {
  Q_ASSERT(numerator_box_pointer != nullptr);
  Q_ASSERT(denominator_box_pointer != nullptr);
  return Rational(numerator_box_pointer->value(),
                  denominator_box_pointer->value());
}

void RationalEditor::set_rational(Rational new_value) const {
  Q_ASSERT(numerator_box_pointer != nullptr);
  numerator_box_pointer->setValue(new_value.numerator);

  Q_ASSERT(denominator_box_pointer != nullptr);
  denominator_box_pointer->setValue(new_value.denominator);
}