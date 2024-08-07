#include "cell_editors/RationalEditor.hpp"

#include <QBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QSpinBox>
#include <memory>

#include "cell_values/Rational.hpp"
#include "other/bounds.hpp"

class QWidget;

static const auto RATIONAL_MARGIN = 2;

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
  row_pointer->setContentsMargins(RATIONAL_MARGIN, RATIONAL_MARGIN,
                                  RATIONAL_MARGIN, RATIONAL_MARGIN);
  setLayout(row_pointer);

  setMinimumSize(sizeHint());
}

auto RationalEditor::value() const -> Rational {
  return Rational(
      {numerator_box_pointer->value(), denominator_box_pointer->value()});
}

void RationalEditor::setValue(Rational new_value) const {
  numerator_box_pointer->setValue(new_value.numerator);
  denominator_box_pointer->setValue(new_value.denominator);
}
