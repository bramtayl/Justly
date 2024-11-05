#include "interval/Interval.hpp"
#include "other/other.hpp"

#include <QtGlobal>
#include <cmath>

static const auto OCTAVE_RATIO = 2.0;

Interval::Interval(const nlohmann::json &json_rational)
    : AbstractRational(json_rational),
      octave(json_rational.value("octave", 0)) {}

Interval::Interval(int numerator, int denominator, int octave_input)
    : AbstractRational(numerator, denominator), octave(octave_input) {}

auto Interval::operator==(const Interval &other_interval) const -> bool {
  return numerator == other_interval.numerator &&
         denominator == other_interval.denominator &&
         octave == other_interval.octave;
}

auto Interval::is_default() const -> bool {
  return AbstractRational::is_default() && octave == 0;
}

auto variant_to_interval(const QVariant &variant) -> Interval {
  Q_ASSERT(variant.canConvert<Interval>());
  return variant.value<Interval>();
}

auto Interval::to_double() const -> double {
  return AbstractRational::to_double() * pow(OCTAVE_RATIO, octave);
}

auto get_interval_schema() -> nlohmann::json {
  return get_object_schema(
      "an interval",
      nlohmann::json(
          {{"numerator", get_number_schema("integer", "numerator", 1,
                                           MAX_RATIONAL_NUMERATOR)},
           {"denominator", get_number_schema("integer", "denominator", 1,
                                             MAX_RATIONAL_DENOMINATOR)},
           {"octave",
            get_number_schema("integer", "octave", -MAX_OCTAVE, MAX_OCTAVE)}}));
}

[[nodiscard]] auto Interval::to_json() const -> nlohmann::json {
  nlohmann::json json_interval;
  add_int_to_json(json_interval, "numerator", numerator, 1);
  add_int_to_json(json_interval, "denominator", denominator, 1);
  add_int_to_json(json_interval, "octave", octave, 0);
  return json_interval;
};

auto json_field_to_interval(const nlohmann::json &json_row) -> Interval {
  if (json_row.contains("interval")) {
    return Interval(json_row["interval"]);
  }
  return {};
}
