#pragma once

#include <QByteArray>
#include <QMetaType>

#include "justly/JUSTLY_EXPORT.hpp"

struct JUSTLY_EXPORT Rational {
  int numerator;
  int denominator;

  explicit Rational(int numerator = 1, int denominator = 1);
  [[nodiscard]] auto operator==(const Rational &other_rational) const -> bool;
};

Q_DECLARE_METATYPE(Rational);

