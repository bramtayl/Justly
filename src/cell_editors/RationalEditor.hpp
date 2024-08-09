#pragma once

#include <QFrame>
#include <QObject>

#include "cell_values/Rational.hpp"

class QSpinBox;
class QWidget;

class RationalEditor : public QFrame {
  Q_OBJECT
  Q_PROPERTY(Rational rational READ value WRITE setValue USER true)

  QSpinBox *const numerator_box_pointer;
  QSpinBox *const denominator_box_pointer;

public:
  explicit RationalEditor(QWidget *parent_pointer_input = nullptr);

  [[nodiscard]] auto value() const -> Rational;
  void setValue(Rational new_value) const;
};
