#pragma once

#include <qframe.h>
#include <qmetatype.h>     // for qRegisterMetaType, qRegisterNormaliz...
#include <qspinbox.h>      // for QSpinBox
#include <qtmetamacros.h>  // for Q_OBJECT

#include "justly/metatypes/Interval.h"

class QWidget;

Q_DECLARE_METATYPE(Interval)

class IntervalEditor : public QFrame {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ value WRITE set_interval USER true)
 private:
  QSpinBox* numerator_box_pointer = new QSpinBox(this);
  QSpinBox* denominator_box_pointer = new QSpinBox(this);
  QSpinBox* octave_box_pointer = new QSpinBox(this);

 public:
  explicit IntervalEditor(QWidget* = nullptr);
  [[nodiscard]] auto value() const -> Interval;
  void set_interval(Interval) const;
};
