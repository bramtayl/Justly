#pragma once

#include <QFrame>

class QSpinBox;
class QWidget;

struct AbstractRationalEditor : public QFrame {
public:
  QSpinBox &numerator_box;
  QSpinBox &denominator_box;
  explicit AbstractRationalEditor(QWidget *parent_pointer);
};
