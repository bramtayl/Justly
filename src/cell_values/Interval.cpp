#include "cell_values/Interval.hpp"

#include <QtGlobal>
#include <cmath>

static const auto OCTAVE_RATIO = 2.0;

auto Interval::operator==(const Interval &other_interval) const -> bool {
  return numerator == other_interval.numerator &&
         denominator == other_interval.denominator &&
         octave == other_interval.octave;
}

auto interval_to_double(const Interval &interval) -> double {
  Q_ASSERT(interval.denominator != 0);
  return (1.0 * interval.numerator) / interval.denominator *
         pow(OCTAVE_RATIO, interval.octave);
}