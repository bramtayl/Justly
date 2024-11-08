#include "abstract_rational/rational/Rational.hpp"

#include <nlohmann/json.hpp>

Rational::Rational(const nlohmann::json &json_rational)
    : AbstractRational(json_rational) {}

Rational::Rational(int numerator, int denominator)
    : AbstractRational(numerator, denominator) {}

auto Rational::operator==(const Rational &other_rational) const -> bool {
  return numerator == other_rational.numerator &&
         denominator == other_rational.denominator;
}

