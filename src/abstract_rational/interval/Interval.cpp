#include "abstract_rational/interval/Interval.hpp"

#include <cmath>

#include "other/other.hpp"

static const auto OCTAVE_RATIO = 2.0;

Interval::Interval(const nlohmann::json &json_rational)
    : AbstractRational(json_rational),
      octave(json_rational.value("octave", 0)) {}

Interval::Interval(int numerator, int denominator, int octave_input)
    : AbstractRational(numerator, denominator), octave(octave_input) {}

auto Interval::operator==(const Interval &other_interval) const -> bool {
  return AbstractRational::operator==(other_interval) &&
         octave == other_interval.octave;
}

auto Interval::is_default() const -> bool {
  return AbstractRational::is_default() && octave == 0;
}

auto Interval::to_double() const -> double {
  return AbstractRational::to_double() * pow(OCTAVE_RATIO, octave);
}

auto get_interval_schema() -> nlohmann::json {
  return get_object_schema(
      "an interval",
      nlohmann::json({{"numerator", get_numerator_schema()},
                      {"denominator", get_denominator_schema()},
                      {"octave", get_number_schema("integer", "octave",
                                                   -MAX_OCTAVE, MAX_OCTAVE)}}));
}

void Interval::to_json(nlohmann::json &json_interval) const {
  AbstractRational::to_json(json_interval);
  add_int_to_json(json_interval, "octave", octave, 0);
};
