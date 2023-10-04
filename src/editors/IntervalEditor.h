#pragma once

#include <qspinbox.h>      // for QSpinBox
#include <qtmetamacros.h>  // for Q_OBJECT
#include <qwidget.h>       // for QWidget

#include <gsl/pointers>
#include <memory>  // for make_unique, __unique_ptr_t

#include "metatypes/Interval.h"  // for Interval

class IntervalEditor : public QWidget {
  Q_OBJECT
 private:
  gsl::not_null<QWidget*> fraction_widget_pointer =
      std::make_unique<QWidget>(this).release();
  gsl::not_null<QSpinBox*> numerator_box_pointer =
      std::make_unique<QSpinBox>(fraction_widget_pointer).release();
  gsl::not_null<QSpinBox*> denominator_box_pointer =
      std::make_unique<QSpinBox>(fraction_widget_pointer).release();
  gsl::not_null<QSpinBox*> octave_box_pointer =
      std::make_unique<QSpinBox>(this).release();
 public:
  explicit IntervalEditor(QWidget* parent_pointer_input = nullptr);
  [[nodiscard]] auto value() const -> Interval;
  void setValue(const Interval& interval) const;
  [[nodiscard]] auto get_numerator() const -> int;
  void set_numerator(int numerator) const;
  [[nodiscard]] auto get_denominator() const -> int;
  void set_denominator(int denominator) const;
  [[nodiscard]] auto get_octave() const -> int;
  void set_octave(int octave) const;
};
