#pragma once

#include <qboxlayout.h>    // for QHBoxLayout, QVBoxLayout
#include <qframe.h>        // for QFrame
#include <qlabel.h>        // for QLabel
#include <qpointer.h>      // for QPointer
#include <qspinbox.h>      // for QSpinBox
#include <qtmetamacros.h>  // for Q_OBJECT
#include <qwidget.h>       // for QWidget

#include "Interval.h"

class IntervalEditor : public QWidget {
  Q_OBJECT
 public:
  const QPointer<QSpinBox> numerator_box_pointer = new QSpinBox();
  const QPointer<QFrame> vinculum_pointer = new QFrame();
  const QPointer<QSpinBox> denominator_box_pointer = new QSpinBox();
  const QPointer<QLabel> power_label = new QLabel("× 2");
  const QPointer<QSpinBox> octave_box_pointer = new QSpinBox();
  const QPointer<QWidget> fraction_widget_pointer = new QWidget();
  const QPointer<QHBoxLayout> row_pointer = new QHBoxLayout();
  const QPointer<QVBoxLayout> column_pointer = new QVBoxLayout();

  explicit IntervalEditor(QWidget* parent_pointer_input = nullptr);
  void set_interval(const Interval& interval);
  [[nodiscard]] auto get_interval() const -> Interval;
};