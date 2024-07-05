#pragma once

#include <qframe.h>        // for QFrame
#include <qspinbox.h>      // for QSpinBox
#include <qtmetamacros.h>  // for Q_OBJECT, Q_PROPERTY
#include <qwidget.h>

#include "justly/Rational.hpp"  // for Rational

class RationalEditor : public QFrame {
  Q_OBJECT
  Q_PROPERTY(Rational rational READ value WRITE setValue USER true)

  QSpinBox* const numerator_box_pointer = new QSpinBox(this);
  QSpinBox* const denominator_box_pointer = new QSpinBox(this);

 public:
  explicit RationalEditor(QWidget* parent_pointer_input = nullptr);

  [[nodiscard]] auto value() const -> Rational;
  void setValue(Rational new_value) const;
};
