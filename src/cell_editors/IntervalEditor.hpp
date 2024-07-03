#pragma once

#include <qframe.h>        // for QFrame
#include <qspinbox.h>      // for QSpinBox
#include <qtmetamacros.h>  // for Q_OBJECT, Q_PROPERTY

#include "justly/Interval.hpp"  // for Interval

class QWidget;

class IntervalEditor : public QFrame {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ value WRITE setValue USER true)

  QSpinBox* const numerator_box_pointer = new QSpinBox(this);
  QSpinBox* const denominator_box_pointer = new QSpinBox(this);
  QSpinBox* const octave_box_pointer = new QSpinBox(this);

 public:
  explicit IntervalEditor(QWidget* parent_pointer_input = nullptr);

  [[nodiscard]] auto value() const -> Interval;
  void setValue(Interval new_value) const;
};
