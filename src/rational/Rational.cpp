#include <nlohmann/json.hpp>

#include "rational/Rational.hpp"

auto Rational::operator==(const Rational &other_rational) const -> bool {
  return numerator == other_rational.numerator &&
         denominator == other_rational.denominator;
}

auto rational_is_default(const Rational &rational) -> bool {
  return rational.numerator == 1 && rational.denominator == 1;
}

auto rational_to_json(const Rational &rational) -> nlohmann::json {
  auto numerator = rational.numerator;
  auto denominator = rational.denominator;

  auto json_rational = nlohmann::json::object();
  if (numerator != 1) {
    json_rational["numerator"] = numerator;
  }
  if (denominator != 1) {
    json_rational["denominator"] = denominator;
  }
  return json_rational;
}

auto json_to_rational(const nlohmann::json &json_rational) -> Rational {
  return Rational({json_rational.value("numerator", 1),
                   json_rational.value("denominator", 1)});
}
