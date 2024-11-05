#include "rational/RationalEditor.hpp"

#include <QBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QString>

#include "other/other.hpp"
#include "rational/Rational.hpp"

class QWidget;

RationalEditor::RationalEditor(QWidget *parent_pointer)
    : AbstractRationalEditor(parent_pointer) {
  auto *row_pointer = // NOLINT(cppcoreguidelines-owning-memory)
      new QHBoxLayout(this);
  row_pointer->addWidget(&numerator_box);
  row_pointer->addWidget(
      new QLabel("/", this)); // NOLINT(cppcoreguidelines-owning-memory)
  row_pointer->addWidget(&denominator_box);

  set_minimum_size(*this);
}

auto RationalEditor::value() const -> Rational {
  return {numerator_box.value(), denominator_box.value()};
}

void RationalEditor::setValue(const Rational &new_value) const {
  numerator_box.setValue(new_value.numerator);
  denominator_box.setValue(new_value.denominator);
}
