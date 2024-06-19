#pragma once

#include <qframe.h>
#include <qmetatype.h>     // for qRegisterMetaType, qRegisterNormaliz...
#include <qspinbox.h>      // for QSpinBox
#include <qtmetamacros.h>  // for Q_OBJECT

#include "justly/global.h"
#include "justly/Interval.h"

class QWidget;

class JUSTLY_EXPORT IntervalEditor : public QFrame {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ get_interval WRITE set_interval USER true)

  QSpinBox* numerator_box_pointer = new QSpinBox(this);
  QSpinBox* denominator_box_pointer = new QSpinBox(this);
  QSpinBox* octave_box_pointer = new QSpinBox(this);

 public:
  explicit IntervalEditor(QWidget* = nullptr);
  [[nodiscard]] auto get_interval() const -> Interval;
  void set_interval(Interval) const;
};
