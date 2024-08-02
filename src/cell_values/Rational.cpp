#include "justly/Rational.hpp"

Rational::Rational(int numerator_input, int denominator_input)
    : numerator(numerator_input), denominator(denominator_input) {}

auto Rational::operator==(const Rational &other_rational) const -> bool {
  return numerator == other_rational.numerator &&
         denominator == other_rational.denominator;
}

