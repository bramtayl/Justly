#include "rational/Rational.hpp"

#include <nlohmann/json.hpp>

#include "other/other.hpp"

Rational::Rational(const nlohmann::json &json_rational)
    : AbstractRational(json_rational) {}

Rational::Rational(int numerator, int denominator)
    : AbstractRational(numerator, denominator) {}

auto Rational::operator==(const Rational &other_rational) const -> bool {
  return numerator == other_rational.numerator &&
         denominator == other_rational.denominator;
}

auto get_rational_schema(const char *description) -> nlohmann::json {
  return get_object_schema(
      description,
      nlohmann::json(
          {{"numerator", get_number_schema("integer", "numerator", 1,
                                           MAX_RATIONAL_NUMERATOR)},
           {"denominator", get_number_schema("integer", "denominator", 1,
                                             MAX_RATIONAL_DENOMINATOR)}}));
}
