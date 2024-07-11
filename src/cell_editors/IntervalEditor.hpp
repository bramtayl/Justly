#pragma once

#include <QFrame>   // for QFrame
#include <QObject>  // for Q_OBJECT, Q_PROPERTY
#include <QSize>
#include <QSpinBox>  // for QSpinBox
#include <QWidget>

#include "justly/Interval.hpp"  // for Interval

[[nodiscard]] auto get_interval_size() -> QSize;

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
