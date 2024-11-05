#pragma once

#include <QObject>

#include "rational/AbstractRationalEditor.hpp"
#include "rational/Rational.hpp"

class QWidget;

struct RationalEditor : public AbstractRationalEditor {
  Q_OBJECT
  Q_PROPERTY(Rational rational READ value WRITE setValue USER true)

public:
  explicit RationalEditor(QWidget *parent_pointer);

  [[nodiscard]] auto value() const -> Rational;
  void setValue(const Rational &new_value) const;
};
