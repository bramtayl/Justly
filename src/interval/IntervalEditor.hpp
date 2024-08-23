#pragma once

#include "interval/Interval.hpp"
#include <QFrame>
#include <QObject>

class QSpinBox;
class QWidget;

class IntervalEditor : public QFrame {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ value WRITE setValue USER true)

  QSpinBox *const numerator_box_pointer;
  QSpinBox *const denominator_box_pointer;
  QSpinBox *const octave_box_pointer;

public:
  explicit IntervalEditor(QWidget *parent_pointer_input = nullptr);

  [[nodiscard]] auto value() const -> Interval;
  void setValue(Interval new_value) const;
};
