#pragma once

#include <qboxlayout.h>  // for QHBoxLayout, QVBoxLayout
#include <qframe.h>      // for QFrame
#include <qlabel.h>      // for QLabel
#include <qspinbox.h>    // for QSpinBox
#include <qtmetamacros.h>   // for Q_OBJECT
#include <qwidget.h>     // for QWidget

#include <memory>  // for make_unique, __unique_ptr_t

#include "metatypes/Interval.h"  // for Interval

class IntervalEditor : public QWidget {
  Q_OBJECT
 public:
  QWidget* fraction_widget_pointer =
      std::make_unique<QWidget>(this).release();
  QSpinBox* numerator_box_pointer =
      std::make_unique<QSpinBox>(fraction_widget_pointer).release();
  QFrame* vinculum_pointer =
      std::make_unique<QFrame>(fraction_widget_pointer).release();
  QSpinBox* denominator_box_pointer =
      std::make_unique<QSpinBox>(fraction_widget_pointer).release();
  QVBoxLayout* column_pointer =
      std::make_unique<QVBoxLayout>(fraction_widget_pointer).release();

  QLabel* power_label = std::make_unique<QLabel>("× 2", this).release();
  QSpinBox* octave_box_pointer =
      std::make_unique<QSpinBox>(this).release();
  QHBoxLayout* row_pointer =
      std::make_unique<QHBoxLayout>(this).release();

  explicit IntervalEditor(QWidget* parent_pointer_input = nullptr);
  void set_interval(const Interval& interval) const;
  [[nodiscard]] auto get_interval() const -> Interval;
};
