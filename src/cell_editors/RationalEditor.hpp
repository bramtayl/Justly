#pragma once

#include <QFrame>    // for QFrame
#include <QObject>   // for Q_OBJECT, Q_PROPERTY
#include <QSize>     // for QSize
#include <QSpinBox>  // for QSpinBox
#include <QWidget>

#include "justly/Rational.hpp"  // for Rational

[[nodiscard]] auto get_rational_size() -> QSize;

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
