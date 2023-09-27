#pragma once

#include <qspinbox.h>    // for QSpinBox
#include <qtmetamacros.h>   // for Q_OBJECT
#include <qwidget.h>     // for QWidget

#include <memory>  // for make_unique, __unique_ptr_t

#include "metatypes/Interval.h"  // for Interval

class IntervalEditor : public QWidget {
  Q_OBJECT
 private:
  QWidget* fraction_widget_pointer =
      std::make_unique<QWidget>(this).release();
 public:
  QSpinBox* numerator_box_pointer =
      std::make_unique<QSpinBox>(fraction_widget_pointer).release();
  QSpinBox* denominator_box_pointer =
      std::make_unique<QSpinBox>(fraction_widget_pointer).release();
  QSpinBox* octave_box_pointer =
      std::make_unique<QSpinBox>(this).release();
  explicit IntervalEditor(QWidget* parent_pointer_input = nullptr);
  void set_interval(const Interval& interval) const;
  [[nodiscard]] auto get_interval() const -> Interval;
};
