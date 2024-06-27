#pragma once

#include <qframe.h>        // for QFrame
#include <qspinbox.h>      // for QSpinBox
#include <qtmetamacros.h>  // for Q_OBJECT, Q_PROPERTY

#include "justly/Rational.hpp"  // for Rational

class QWidget;

class RationalEditor : public QFrame {
  Q_OBJECT
  Q_PROPERTY(Rational rational READ get_rational WRITE set_rational USER true)

  QSpinBox* numerator_box_pointer = new QSpinBox(this);
  QSpinBox* denominator_box_pointer = new QSpinBox(this);

 public:
  explicit RationalEditor(QWidget* parent_pointer_input = nullptr);

  [[nodiscard]] auto get_rational() const -> Rational;
  void set_rational(Rational new_value) const;
};
