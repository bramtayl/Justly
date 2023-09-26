#include "metatypes/Interval.h"

#include <qstring.h>  // for QString

#include <cmath>                         // for pow
#include <map>                           // for operator!=
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json

Interval::Interval(int numerator_input, int denominator_input, int octave_input)
    : numerator(numerator_input),
      denominator(denominator_input),
      octave(octave_input) {}

auto Interval::get_text() const -> QString {
  if (denominator == DEFAULT_DENOMINATOR) {
    if (octave == DEFAULT_OCTAVE) {
      return QString("%1").arg(numerator);
    }
    return QString("%1o%2").arg(numerator).arg(octave);
  }
  if (octave == DEFAULT_OCTAVE) {
    return QString("%1/%2").arg(numerator).arg(denominator);
  }
  return QString("%1/%2o%3").arg(numerator).arg(denominator).arg(octave);
}

auto Interval::is_default() const -> bool {
  return numerator == DEFAULT_NUMERATOR && denominator == DEFAULT_DENOMINATOR &&
         octave == DEFAULT_OCTAVE;
}

auto Interval::get_ratio() const -> double {
  return (1.0 * numerator) / denominator * pow(OCTAVE_RATIO, octave);
}

auto Interval::operator==(const Interval &other_interval) const -> bool {
  return numerator == other_interval.numerator &&
         denominator == other_interval.denominator &&
         octave == other_interval.octave;
}

auto Interval::get_schema() -> const nlohmann::json & {
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

auto Interval::save_to(nlohmann::json &json_map) const -> void {
  if (numerator != DEFAULT_NUMERATOR) {
    json_map["numerator"] = numerator;
  }
  if (denominator != DEFAULT_DENOMINATOR) {
    json_map["denominator"] = denominator;
  }
  if (octave != DEFAULT_OCTAVE) {
    json_map["octave"] = octave;
  }
}

void Interval::load_from(const nlohmann::json &json_interval) {
  if (json_interval.contains("numerator")) {
    numerator = json_interval["numerator"].get<int>();
  }
  if (json_interval.contains("denominator")) {
    denominator = json_interval["denominator"].get<int>();
  }
  if (json_interval.contains("octave")) {
    octave = json_interval["octave"].get<int>();
  }
}
