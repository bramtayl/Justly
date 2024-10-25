#include "interval/Interval.hpp"

#include <QtGlobal>
#include <cmath>

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
