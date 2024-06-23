#include "justly/Rational.hpp"

#include <cmath>                  // for pow
#include <map>                    // for operator!=, operator==
#include <nlohmann/json.hpp>      // for basic_json<>::object_t, basic_json
#include <nlohmann/json_fwd.hpp>  // for json
#include <sstream>                // for basic_ostream::operator<<, operator<<

Rational::Rational(int numerator_input, int denominator_input)
    : numerator(numerator_input),
      denominator(denominator_input) {}

Rational::Rational(const nlohmann::json& json_rational)
    : numerator(json_rational.value("numerator", DEFAULT_NUMERATOR)),
      denominator(json_rational.value("denominator", DEFAULT_DENOMINATOR)) {}

auto Rational::operator==(const Rational& other_rational) const -> bool {
  return numerator == other_rational.numerator &&
         denominator == other_rational.denominator;
}

auto Rational::is_default() const -> bool {
  return numerator == DEFAULT_NUMERATOR && denominator == DEFAULT_DENOMINATOR;
}

auto Rational::ratio() const -> double {
  return (1.0 * numerator) / denominator;
}

auto Rational::text() const -> std::string {
  std::stringstream interval_io;
  interval_io << numerator;
  if (denominator != DEFAULT_DENOMINATOR) {
    interval_io << "/" << denominator;
  }
  return interval_io.str();
}

auto Rational::json() const -> nlohmann::json {
  auto json_rational = nlohmann::json::object();
  if (numerator != DEFAULT_NUMERATOR) {
    json_rational["numerator"] = numerator;
  }
  if (denominator != DEFAULT_DENOMINATOR) {
    json_rational["denominator"] = denominator;
  }
  return json_rational;
}
