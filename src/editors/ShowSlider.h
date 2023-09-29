#pragma once

#include <qboxlayout.h>    // for QHBoxLayout
#include <qslider.h>       // for QSlider
#include <qspinbox.h>      // for QSpinBox
#include <qstring.h>       // for QString
#include <qtmetamacros.h>  // for Q_OBJECT
#include <qwidget.h>       // for QWidget

#include <gsl/pointers>
#include <memory>  // for make_unique, __unique_ptr_t

class ShowSlider : public QWidget {
  Q_OBJECT
 private:
  int minimum;
  int maximum;
  QString suffix;
    QHBoxLayout* layout_pointer = std::make_unique<QHBoxLayout>(this).release();
  gsl::not_null<QSpinBox*> spin_box_pointer =
    std::make_unique<QSpinBox>(this).release();
  gsl::not_null<QSlider*> slider_pointer =
      std::make_unique<QSlider>(this).release();

 public:
  explicit ShowSlider(int minimum, int maximum, const QString& suffix,
                      QWidget* parent_pointer = nullptr);
  [[nodiscard]] auto value() const -> double;
  void setValue(double new_value) const;
  void set_value_no_signals(double new_value) const;
 signals:
  void valueChanged();
};
