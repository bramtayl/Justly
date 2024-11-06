#pragma once

#include <QFrame>

struct AbstractRational;
class QSpinBox;
class QWidget;

struct AbstractRationalEditor : public QFrame {
public:
  QSpinBox &numerator_box;
  QSpinBox &denominator_box;
  explicit AbstractRationalEditor(QWidget *parent_pointer);

  void setValue(const AbstractRational &new_value) const;
};
