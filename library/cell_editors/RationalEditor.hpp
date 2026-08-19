#pragma once

#include <QtCore/qtmetamacros.h>

#include <QtCore/QString>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>

#include "cell_types/Rational.hpp"

class QWidget;

struct RationalEditor : QFrame {
  Q_OBJECT
  Q_PROPERTY(Rational rational READ value WRITE setValue USER true)

 public:
  QSpinBox& numerator_box = *(new QSpinBox);
  QLabel& slash_text = *(new QLabel("/"));
  QSpinBox& denominator_box = *(new QSpinBox);
  QBoxLayout& row_layout = *(new QHBoxLayout(this));

  explicit RationalEditor(QWidget* parent_pointer);

  [[nodiscard]] auto value() const -> Rational;

  void setValue(const Rational& new_value) const;
};
