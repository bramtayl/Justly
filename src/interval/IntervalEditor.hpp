#pragma once

#include "interval/Interval.hpp"
#include <QObject>

#include "rational/AbstractRationalEditor.hpp"

class QSpinBox;
class QWidget;

struct IntervalEditor : public AbstractRationalEditor {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ value WRITE setValue USER true)

public:
  QSpinBox &octave_box;

  explicit IntervalEditor(QWidget *parent_pointer_input);

  [[nodiscard]] auto value() const -> Interval;
  void setValue(const Interval &new_value) const;
};
