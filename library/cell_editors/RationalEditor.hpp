#pragma once

#include <QtWidgets/QFrame>

#include "cell_types/Rational.hpp"

class QSpinBox;

struct RationalEditor : QFrame {
  Q_OBJECT
  Q_PROPERTY(Rational rational READ value WRITE setValue USER true)

 public:
  QSpinBox& numerator_box;
  QSpinBox& denominator_box;

  explicit RationalEditor(QWidget* parent_pointer);

  [[nodiscard]] auto value() const -> Rational;

  void setValue(const Rational& new_value) const;
};
