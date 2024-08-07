#pragma once

#include <QByteArray>
#include <QMetaType>

struct Rational {
  int numerator;
  int denominator;

  explicit Rational(int numerator = 1, int denominator = 1);
  [[nodiscard]] auto operator==(const Rational &other_rational) const -> bool;
};

Q_DECLARE_METATYPE(Rational);

