#include "ShowSlider.h"

#include <qabstractslider.h>  // for QAbstractSlider
#include <qnamespace.h>       // for Horizontal

const auto SMALLER_MARGIN = 5;

ShowSlider::ShowSlider(int minimum, int maximum, const QString& suffix,
                       QWidget* parent)
    : QWidget(parent), minimum(minimum), maximum(maximum), suffix(suffix) {
  slider.setMinimum(minimum);
  spin_box.setMinimum(minimum);
  slider.setMaximum(maximum);
  spin_box.setMaximum(maximum);
  spin_box.setSuffix(suffix);
  slider.setOrientation(Qt::Horizontal);
  slider.setAutoFillBackground(true);

  layout.addWidget(&slider);
  layout.addWidget(&spin_box);
  layout.setContentsMargins(SMALLER_MARGIN, SMALLER_MARGIN, SMALLER_MARGIN,
                            SMALLER_MARGIN);
  setLayout(&layout);
  setAutoFillBackground(true);

  connect(&slider, &QAbstractSlider::valueChanged, &spin_box,
          &QSpinBox::setValue);
  connect(&spin_box, &QSpinBox::valueChanged, &slider,
          &QAbstractSlider::setValue);
};
