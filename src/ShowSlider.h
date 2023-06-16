#pragma once

#include <qboxlayout.h>    // for QHBoxLayout
#include <qpointer.h>      // for QPointer
#include <qslider.h>       // for QSlider
#include <qspinbox.h>      // for QSpinBox
#include <qstring.h>       // for QString
#include <qtmetamacros.h>  // for Q_OBJECT
#include <qwidget.h>       // for QWidget

class ShowSlider : public QWidget {
  Q_OBJECT
 public:
  const int minimum;
  const int maximum;
  const QString suffix;
  const QPointer<QSpinBox> spin_box_pointer = new QSpinBox();
  const QPointer<QSlider> slider_pointer = new QSlider();
  const QPointer<QHBoxLayout> layout_pointer = new QHBoxLayout();
  
  explicit ShowSlider(int minimum, int maximum, const QString& suffix,
             QWidget* parent = nullptr);
  void set_value_override(double new_value);
};