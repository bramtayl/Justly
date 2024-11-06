#pragma once

#include <QFrame>

struct AbstractRational;
class QHBoxLayout;
class QSpinBox;
class QWidget;

struct AbstractRationalEditor : public QFrame {
public:
  QSpinBox &numerator_box;
  QSpinBox &denominator_box;
  QHBoxLayout &row_layout;
  
  explicit AbstractRationalEditor(QWidget *parent_pointer);

  void setValue(const AbstractRational &new_value) const;
};
