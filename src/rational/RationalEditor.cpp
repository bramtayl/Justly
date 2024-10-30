#include "rational/RationalEditor.hpp"

#include <QBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QSpinBox>
#include <QString>

#include "rational/Rational.hpp"

class QWidget;

RationalEditor::RationalEditor(QWidget *parent_pointer_input)
    : QFrame(parent_pointer_input), numerator_box(*(new QSpinBox(this))),
      denominator_box(*(new QSpinBox(this))) {
  setFrameStyle(QFrame::StyledPanel);
  setAutoFillBackground(true);

  numerator_box.setMinimum(1);
  numerator_box.setMaximum(MAX_RATIONAL_NUMERATOR);

  denominator_box.setMinimum(1);
  denominator_box.setMaximum(MAX_RATIONAL_DENOMINATOR);

  auto *row_pointer = // NOLINT(cppcoreguidelines-owning-memory)
      new QHBoxLayout(this);
  row_pointer->addWidget(&numerator_box);
  row_pointer->addWidget(
      new QLabel("/", this)); // NOLINT(cppcoreguidelines-owning-memory)
  row_pointer->addWidget(&denominator_box);

  setMinimumSize(minimumSizeHint());
}

auto RationalEditor::value() const -> Rational {
  return Rational(
      {numerator_box.value(), denominator_box.value()});
}

void RationalEditor::setValue(Rational new_value) const {
  numerator_box.setValue(new_value.numerator);
  denominator_box.setValue(new_value.denominator);
}
