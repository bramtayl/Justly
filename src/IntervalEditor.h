#pragma once

#include <qframe.h>        // for QFrame
#include <qspinbox.h>      // for QSpinBox
#include <qtmetamacros.h>  // for Q_OBJECT, Q_PROPERTY

#include "justly/Interval.h"  // for Interval
#include "justly/global.h"    // for JUSTLY_EXPORT

class QWidget;

class JUSTLY_EXPORT IntervalEditor : public QFrame {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ get_interval WRITE set_interval USER true)

  QSpinBox* numerator_box_pointer = new QSpinBox(this);
  QSpinBox* denominator_box_pointer = new QSpinBox(this);
  QSpinBox* octave_box_pointer = new QSpinBox(this);

 public:
  explicit IntervalEditor(QWidget* parent_pointer_input = nullptr);

  [[nodiscard]] auto get_interval() const -> Interval;
  void set_interval(Interval new_value) const;
};
