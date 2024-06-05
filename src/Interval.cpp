#include "justly/Interval.h"

#include <cmath>                         // for pow
#include <map>                           // for operator!=
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json
#include <ostream>

const auto OCTAVE_RATIO = 2.0;

Interval::Interval(int numerator_input, int denominator_input, int octave_input)
    : numerator(numerator_input),
      denominator(denominator_input),
      octave(octave_input) {}

Interval::Interval(const nlohmann::json& json_interval)
    : numerator(json_interval.value("numerator", DEFAULT_NUMERATOR)),
      denominator(json_interval.value("denominator", DEFAULT_DENOMINATOR)),
      octave(json_interval.value("octave", DEFAULT_OCTAVE)) {}

auto Interval::get_text() const -> std::string {
  std::stringstream interval_io;
  interval_io << numerator;
  if (denominator != DEFAULT_DENOMINATOR) {
    interval_io << "/" << denominator;
  }
  if (octave != DEFAULT_OCTAVE) {
    interval_io << "o" << octave;
  }
  return interval_io.str();
}

auto Interval::is_default() const -> bool {
  return numerator == DEFAULT_NUMERATOR && denominator == DEFAULT_DENOMINATOR &&
         octave == DEFAULT_OCTAVE;
}

auto Interval::ratio() const -> double {
  return (1.0 * numerator) / denominator * pow(OCTAVE_RATIO, octave);
}

auto Interval::operator==(const Interval& other_interval) const -> bool {
  return numerator == other_interval.numerator &&
         denominator == other_interval.denominator &&
         octave == other_interval.octave;
}

auto Interval::json_schema() -> const nlohmann::json& {
  static const nlohmann::json interval_schema(
      {{"type", "object"},
       {"description", "an interval"},
       {"properties",
        {{"numerator",
          {{"type", "integer"},
           {"description", "the numerator"},
           {"minimum", MINIMUM_NUMERATOR},
           {"maximum", MAXIMUM_NUMERATOR}}},
         {"denominator",
          {{"type", "integer"},
           {"description", "the denominator"},
           {"minimum", MINIMUM_DENOMINATOR},
           {"maximum", MAXIMUM_DENOMINATOR}}},
         {"octave",
          {{"type", "integer"},
           {"description", "the octave"},
           {"minimum", MINIMUM_OCTAVE},
           {"maximum", MAXIMUM_OCTAVE}}}}}});
  return interval_schema;
}

auto Interval::json() const -> nlohmann::json {
  auto json_interval = nlohmann::json::object();
  if (numerator != DEFAULT_NUMERATOR) {
    json_interval["numerator"] = numerator;
  }
  if (denominator != DEFAULT_DENOMINATOR) {
    json_interval["denominator"] = denominator;
  }
  if (octave != DEFAULT_OCTAVE) {
    json_interval["octave"] = octave;
  }
  return json_interval;
}
