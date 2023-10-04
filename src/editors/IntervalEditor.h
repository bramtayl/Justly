#pragma once

#include <qframe.h>
#include <qspinbox.h>      // for QSpinBox
#include <qtmetamacros.h>  // for Q_OBJECT
#include <qwidget.h>       // for QWidget

#include <gsl/pointers>  // for not_null
#include <memory>        // for make_unique, __unique_ptr_t

#include "metatypes/Interval.h"

class IntervalEditor : public QFrame {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ value WRITE setValue USER true) 
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
  void setValue(Interval new_value) const;
  [[nodiscard]] auto get_numerator() const -> int;
  void set_numerator(int numerator) const;
  [[nodiscard]] auto get_denominator() const -> int;
  void set_denominator(int denominator) const;
  [[nodiscard]] auto get_octave() const -> int;
  void set_octave(int octave) const;
};
