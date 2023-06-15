#pragma once

#include <qboxlayout.h>    // for QHBoxLayout
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
  QSpinBox spin_box;
  QSlider slider;
  QHBoxLayout layout;
  
  explicit ShowSlider(int minimum, int maximum, const QString& suffix,
             QWidget* parent = nullptr);
  void set_value_override(int new_value);
};