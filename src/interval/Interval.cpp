#include "interval/Interval.hpp"
#include "other/other.hpp"

#include <QtGlobal>
#include <cmath>
#include <utility>

static const auto OCTAVE_RATIO = 2.0;

auto Interval::operator==(const Interval &other_interval) const -> bool {
  return numerator == other_interval.numerator &&
         denominator == other_interval.denominator &&
         octave == other_interval.octave;
}

auto variant_to_interval(const QVariant &variant) -> Interval {
  Q_ASSERT(variant.canConvert<Interval>());
  return variant.value<Interval>();
}

auto interval_to_double(const Interval &interval) -> double {
  Q_ASSERT(interval.denominator != 0);
  return (1.0 * interval.numerator) / interval.denominator *
         pow(OCTAVE_RATIO, interval.octave);
}

auto get_interval_schema() -> nlohmann::json {
  return get_object_schema(
      "an interval",
      nlohmann::json(
          {{"numerator", get_number_schema("integer", "numerator", 1,
                                           MAX_INTERVAL_NUMERATOR)},
           {"denominator", get_number_schema("integer", "denominator", 1,
                                             MAX_INTERVAL_DENOMINATOR)},
           {"octave",
            get_number_schema("integer", "octave", MIN_OCTAVE, MAX_OCTAVE)}}));
}

void add_interval_to_json(nlohmann::json &json_row, const Interval &interval) {
  auto numerator = interval.numerator;
  auto denominator = interval.denominator;
  auto octave = interval.octave;
  if (numerator != 1 || denominator != 1 || octave != 0) {
    nlohmann::json json_interval;
    add_int_to_json(json_interval, "numerator", numerator, 1);
    add_int_to_json(json_interval, "denominator", denominator, 1);
    add_int_to_json(json_interval, "octave", octave, 0);
    json_row["interval"] = std::move(json_interval);
  }
}

auto json_field_to_interval(const nlohmann::json &json_row) -> Interval {
  if (json_row.contains("interval")) {
    const auto &json_interval = json_row["interval"];
    return {json_interval.value("numerator", 1),
            json_interval.value("denominator", 1),
            json_interval.value("octave", 0)};
  }
  return {};
}
