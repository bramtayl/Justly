#include "interval/Interval.hpp"

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
  return nlohmann::json(
      {{"type", "object"},
       {"description", "an interval"},
       {"properties",
        nlohmann::json(
            {{"numerator",
              {{"type", "integer"},
               {"description", "the numerator"},
               {"minimum", 1},
               {"maximum", MAX_INTERVAL_NUMERATOR}}},
             {"denominator",
              nlohmann::json({{"type", "integer"},
                              {"description", "the denominator"},
                              {"minimum", 1},
                              {"maximum", MAX_INTERVAL_DENOMINATOR}})},
             {"octave", nlohmann::json({{"type", "integer"},
                                        {"description", "the octave"},
                                        {"minimum", MIN_OCTAVE},
                                        {"maximum", MAX_OCTAVE}})}})}});
}

void add_interval_to_json(nlohmann::json &json_row, const Interval &interval) {
  if (interval.numerator != 1 || interval.denominator != 1 ||
      interval.octave != 0) {
    auto numerator = interval.numerator;
    auto denominator = interval.denominator;
    auto octave = interval.octave;
    auto json_interval = nlohmann::json::object();
    if (interval.numerator != 1) {
      json_interval["numerator"] = numerator;
    }
    if (denominator != 1) {
      json_interval["denominator"] = denominator;
    }
    if (octave != 0) {
      json_interval["octave"] = octave;
    }
    json_row["interval"] = std::move(json_interval);
  }
}
