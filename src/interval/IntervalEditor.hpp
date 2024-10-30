#pragma once

#include "interval/Interval.hpp"
#include <QFrame>
#include <QObject>

class QSpinBox;
class QWidget;

struct IntervalEditor : public QFrame {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ value WRITE setValue USER true)

public:
  QSpinBox& numerator_box;
  QSpinBox& denominator_box;
  QSpinBox& octave_box;

  explicit IntervalEditor(QWidget *parent_pointer_input = nullptr);

  [[nodiscard]] auto value() const -> Interval;
  void setValue(Interval new_value) const;
};
