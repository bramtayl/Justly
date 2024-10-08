#pragma once

#include <QFrame>
#include <QObject>

#include "rational/Rational.hpp"

class QSpinBox;
class QWidget;

struct RationalEditor : public QFrame {
  Q_OBJECT
  Q_PROPERTY(Rational rational READ value WRITE setValue USER true)

public:
  QSpinBox *const numerator_box_pointer;
  QSpinBox *const denominator_box_pointer;

  explicit RationalEditor(QWidget *parent_pointer_input = nullptr);

  [[nodiscard]] auto value() const -> Rational;
  void setValue(Rational new_value) const;
};
