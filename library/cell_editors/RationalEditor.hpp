#pragma once

#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>

#include "cell_types/Rational.hpp"

struct RationalEditor : QFrame {
  Q_OBJECT
  Q_PROPERTY(Rational rational READ value WRITE setValue USER true)

public:
  QSpinBox &numerator_box = *(new QSpinBox);
  QLabel &slash_text = *(new QLabel("/"));
  QSpinBox &denominator_box = *(new QSpinBox);
  QBoxLayout &row_layout = *(new QHBoxLayout(this));

  explicit RationalEditor(QWidget *const parent_pointer)
      : QFrame(parent_pointer) {
    setFrameStyle(QFrame::StyledPanel);
    setAutoFillBackground(true);

    numerator_box.setMinimum(1);
    numerator_box.setMaximum(MAX_NUMERATOR);

    denominator_box.setMinimum(1);
    denominator_box.setMaximum(MAX_DENOMINATOR);

    row_layout.addWidget(&numerator_box);
    row_layout.addWidget(&slash_text);
    row_layout.addWidget(&denominator_box);
    row_layout.setContentsMargins(1, 0, 1, 0);
  }

  [[nodiscard]] auto value() const {
    return Rational(numerator_box.value(), denominator_box.value());
  }

  void setValue(const Rational &new_value) const {
    numerator_box.setValue(new_value.numerator);
    denominator_box.setValue(new_value.denominator);
  }
};