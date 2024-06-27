#include "justly/Rational.hpp"

#include <qassert.h>  // for Q_ASSERT

#include <map>                    // for operator!=, operator==
#include <nlohmann/json.hpp>      // for basic_json<>::object_t, basic_json
#include <nlohmann/json_fwd.hpp>  // for json
#include <sstream>                // for basic_ostream::operator<<, stringst...

Rational::Rational(int numerator_input, int denominator_input)
    : numerator(numerator_input), denominator(denominator_input) {}

Rational::Rational(const nlohmann::json& json_rational)
    : numerator(json_rational.value("numerator", 1)),
      denominator(json_rational.value("denominator", 1)) {}

auto Rational::operator==(const Rational& other_rational) const -> bool {
  return numerator == other_rational.numerator &&
         denominator == other_rational.denominator;
}

auto Rational::is_default() const -> bool {
  return numerator == 1 && denominator == 1;
}

auto Rational::ratio() const -> double {
  Q_ASSERT(denominator != 0);
  return (1.0 * numerator) / denominator;
}

auto Rational::text() const -> std::string {
  std::stringstream interval_io;
  interval_io << numerator;
  if (denominator != 1) {
    interval_io << "/" << denominator;
  }
  return interval_io.str();
}

auto Rational::json() const -> nlohmann::json {
  auto json_rational = nlohmann::json::object();
  if (numerator != 1) {
    json_rational["numerator"] = numerator;
  }
  if (denominator != 1) {
    json_rational["denominator"] = denominator;
  }
  return json_rational;
}
