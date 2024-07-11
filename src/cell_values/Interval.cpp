#include "justly/Interval.hpp"

#include <QtGlobal>           // for Q_ASSERT
#include <cmath>              // for pow
#include <nlohmann/json.hpp>  // for basic_json<>::object_t, basi...
#include <sstream>            // for basic_ostream::operator<<

#include "other/private_constants.hpp"

const auto OCTAVE_RATIO = 2.0;

Interval::Interval(int numerator_input, int denominator_input, int octave_input)
    : numerator(numerator_input),
      denominator(denominator_input),
      octave(octave_input) {}

Interval::Interval(const nlohmann::json& json_interval)
    : numerator(json_interval.value("numerator", 1)),
      denominator(json_interval.value("denominator", 1)),
      octave(json_interval.value("octave", 0)) {}

auto Interval::operator==(const Interval& other_interval) const -> bool {
  return numerator == other_interval.numerator &&
         denominator == other_interval.denominator &&
         octave == other_interval.octave;
}

auto Interval::is_default() const -> bool {
  return numerator == 1 && denominator == 1 && octave == 0;
}

auto Interval::ratio() const -> double {
  Q_ASSERT(denominator != 0);
  return (1.0 * numerator) / denominator * pow(OCTAVE_RATIO, octave);
}

auto Interval::text() const -> std::string {
  std::stringstream interval_io;
  interval_io << numerator;
  if (denominator != 1) {
    interval_io << "/" << denominator;
  }
  if (octave != 0) {
    interval_io << "o" << octave;
  }
  return interval_io.str();
}

auto Interval::json() const -> nlohmann::json {
  auto json_interval = nlohmann::json::object();
  if (numerator != 1) {
    json_interval["numerator"] = numerator;
  }
  if (denominator != 1) {
    json_interval["denominator"] = denominator;
  }
  if (octave != 0) {
    json_interval["octave"] = octave;
  }
  return json_interval;
}

auto get_interval_schema() -> const nlohmann::json& {
  static const nlohmann::json interval_schema(
      {{"type", "object"},
       {"description", "an interval"},
       {"properties",
        {{"numerator",
          {{"type", "integer"},
           {"description", "the numerator"},
           {"minimum", 1},
           {"maximum", MAX_NUMERATOR}}},
         {"denominator",
          {{"type", "integer"},
           {"description", "the denominator"},
           {"minimum", 1},
           {"maximum", MAX_DENOMINATOR}}},
         {"octave",
          {{"type", "integer"},
           {"description", "the octave"},
           {"minimum", MIN_OCTAVE},
           {"maximum", MAX_OCTAVE}}}}}});
  return interval_schema;
}