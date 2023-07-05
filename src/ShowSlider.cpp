#include "ShowSlider.h"

#include <qabstractslider.h>  // for QAbstractSlider
#include <qnamespace.h>       // for Horizontal

const auto SMALLER_MARGIN = 5;

ShowSlider::ShowSlider(int minimum, int maximum, const QString &suffix,
                       QWidget *parent)
    : QWidget(parent), minimum(minimum), maximum(maximum), suffix(suffix) {
  slider_pointer->setMinimum(minimum);
  spin_box_pointer->setMinimum(minimum);
  slider_pointer->setMaximum(maximum);
  spin_box_pointer->setMaximum(maximum);
  spin_box_pointer->setSuffix(suffix);
  slider_pointer->setOrientation(Qt::Horizontal);
  slider_pointer->setAutoFillBackground(true);

  layout_pointer->addWidget(slider_pointer);
  layout_pointer->addWidget(spin_box_pointer);
  layout_pointer->setContentsMargins(SMALLER_MARGIN, SMALLER_MARGIN,
                                     SMALLER_MARGIN, SMALLER_MARGIN);
  setLayout(layout_pointer);
  setAutoFillBackground(true);

  connect(slider_pointer, &QAbstractSlider::valueChanged, spin_box_pointer,
          &QSpinBox::setValue);
  connect(spin_box_pointer, &QSpinBox::valueChanged, slider_pointer,
          &QAbstractSlider::setValue);
};

void ShowSlider::set_value_override(double new_value) {
  slider_pointer->blockSignals(true);
  slider_pointer->setValue(static_cast<int>(new_value));
  slider_pointer->blockSignals(false);
  spin_box_pointer->blockSignals(true);
  spin_box_pointer->setValue(static_cast<int>(new_value));
  spin_box_pointer->blockSignals(false);
}
