#include "editors/ShowSlider.h"

#include <qabstractslider.h>  // for QAbstractSlider
#include <qboxlayout.h>       // for QHBoxLayout
#include <qnamespace.h>       // for Horizontal
#include <qslider.h>          // for QSlider
#include <qspinbox.h>         // for QSpinBox
#include <qstring.h>          // for QString
#include <qwidget.h>          // for QWidget

ShowSlider::ShowSlider(int minimum, int maximum, const QString &suffix,
                       QWidget *parent_pointer)
    : QWidget(parent_pointer),
      minimum(minimum),
      maximum(maximum),
      suffix(suffix) {
  slider_pointer->setMinimum(minimum);
  spin_box_pointer->setMinimum(minimum);
  slider_pointer->setMaximum(maximum);
  spin_box_pointer->setMaximum(maximum);
  spin_box_pointer->setSuffix(suffix);
  slider_pointer->setOrientation(Qt::Horizontal);
  slider_pointer->setAutoFillBackground(true);

  layout_pointer->addWidget(slider_pointer);
  layout_pointer->addWidget(spin_box_pointer);

  setLayout(layout_pointer);
  setAutoFillBackground(true);

  connect(slider_pointer, &QAbstractSlider::valueChanged, spin_box_pointer,
          &QSpinBox::setValue);
  connect(slider_pointer, &QAbstractSlider::valueChanged, this,
          &ShowSlider::valueChanged);
  connect(spin_box_pointer, &QSpinBox::valueChanged, slider_pointer,
          &QAbstractSlider::setValue);
}

auto ShowSlider::value() const -> double {
  return slider_pointer->value();
}

void ShowSlider::setValue(double new_value) const {
  slider_pointer->setValue(static_cast<int>(new_value));
}

void ShowSlider::set_value_no_signals(double new_value) const {
  slider_pointer->blockSignals(true);
  slider_pointer->setValue(static_cast<int>(new_value));
  slider_pointer->blockSignals(false);
  spin_box_pointer->blockSignals(true);
  spin_box_pointer->setValue(static_cast<int>(new_value));
  spin_box_pointer->blockSignals(false);
}