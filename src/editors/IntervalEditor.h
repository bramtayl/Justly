#pragma once

#include <qframe.h>
#include <qmetatype.h>     // for qRegisterMetaType, qRegisterNormaliz...
#include <qspinbox.h>      // for QSpinBox
#include <qtmetamacros.h>  // for Q_OBJECT

#include <gsl/pointers>  // for not_null

#include "justly/metatypes/Interval.h"

class QWidget;

Q_DECLARE_METATYPE(Interval)

class IntervalEditor : public QFrame {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ value WRITE set_interval USER true)
 private:
  gsl::not_null<QSpinBox*> numerator_box_pointer = new QSpinBox(this);
  gsl::not_null<QSpinBox*> denominator_box_pointer = new QSpinBox(this);
  gsl::not_null<QSpinBox*> octave_box_pointer = new QSpinBox(this);

 public:
  explicit IntervalEditor(QWidget* = nullptr);
  [[nodiscard]] auto value() const -> Interval;
  void set_interval(Interval) const;
};
