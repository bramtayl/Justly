#include "cell_editors/RationalEditor.hpp"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>

RationalEditor::RationalEditor(QWidget* const parent_pointer)
    : QFrame(parent_pointer),
      numerator_box(*(new QSpinBox)),
      denominator_box(*(new QSpinBox)) {
  setFrameStyle(QFrame::StyledPanel);
  setAutoFillBackground(true);

  numerator_box.setMinimum(1);
  numerator_box.setMaximum(MAX_NUMERATOR);

  denominator_box.setMinimum(1);
  denominator_box.setMaximum(MAX_DENOMINATOR);

  auto& row_layout =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QHBoxLayout(this));
  row_layout.addWidget(&numerator_box);
  row_layout.addWidget(
      new QLabel("/"));  // NOLINT(cppcoreguidelines-owning-memory)
  row_layout.addWidget(&denominator_box);
  row_layout.setContentsMargins(1, 0, 1, 0);
}

auto RationalEditor::value() const -> Rational {
  return Rational(numerator_box.value(), denominator_box.value());
}

void RationalEditor::setValue(const Rational& new_value) const {
  numerator_box.setValue(new_value.numerator);
  denominator_box.setValue(new_value.denominator);
}
