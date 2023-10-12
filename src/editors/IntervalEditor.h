#pragma once

#include <qframe.h>
#include <qspinbox.h>      // for QSpinBox
#include <qtmetamacros.h>  // for Q_OBJECT

#include <gsl/pointers>  // for not_null
#include <memory>        // for make_unique, __unique_ptr_t

#include "metatypes/Interval.h"

class QWidget;

class IntervalEditor : public QFrame {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ value WRITE setValue USER true)
 private:
  gsl::not_null<QSpinBox*> numerator_box_pointer =
      std::make_unique<QSpinBox>(this).release();
  gsl::not_null<QSpinBox*> denominator_box_pointer =
      std::make_unique<QSpinBox>(this).release();
  gsl::not_null<QSpinBox*> octave_box_pointer =
      std::make_unique<QSpinBox>(this).release();

 public:
  explicit IntervalEditor(QWidget* parent_pointer_input = nullptr);
  [[nodiscard]] auto value() const -> Interval;
  void setValue(Interval new_value) const;
};
