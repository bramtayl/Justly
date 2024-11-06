#pragma once

#include <QObject>

#include "abstract_rational/AbstractRationalEditor.hpp"
#include "abstract_rational/rational/Rational.hpp"

class QWidget;

struct RationalEditor : public AbstractRationalEditor {
  Q_OBJECT
  Q_PROPERTY(Rational rational READ value WRITE setValue USER true)

public:
  explicit RationalEditor(QWidget *parent_pointer);

  [[nodiscard]] auto value() const -> Rational;
};
