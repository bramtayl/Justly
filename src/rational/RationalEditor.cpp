#include "rational/RationalEditor.hpp"

#include <QBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QSpinBox>
#include <memory>

#include "rational/Rational.hpp"

class QWidget;

RationalEditor::RationalEditor(QWidget *parent_pointer_input)
    : QFrame(parent_pointer_input), numerator_box_pointer(new QSpinBox(this)),
      denominator_box_pointer(new QSpinBox(this)) {
  setFrameStyle(QFrame::StyledPanel);
  setAutoFillBackground(true);

  numerator_box_pointer->setMinimum(1);
  numerator_box_pointer->setMaximum(MAX_RATIONAL_NUMERATOR);

  denominator_box_pointer->setMinimum(1);
  denominator_box_pointer->setMaximum(MAX_RATIONAL_DENOMINATOR);

  auto *row_pointer = std::make_unique<QHBoxLayout>(this).release();
  row_pointer->addWidget(numerator_box_pointer);
  row_pointer->addWidget(std::make_unique<QLabel>("/", this).release());
  row_pointer->addWidget(denominator_box_pointer);
  setLayout(row_pointer);

  setMinimumSize(minimumSizeHint());
}

auto RationalEditor::value() const -> Rational {
  return Rational(
      {numerator_box_pointer->value(), denominator_box_pointer->value()});
}

void RationalEditor::setValue(Rational new_value) const {
  numerator_box_pointer->setValue(new_value.numerator);
  denominator_box_pointer->setValue(new_value.denominator);
}
