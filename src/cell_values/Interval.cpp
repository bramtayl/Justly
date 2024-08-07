#include "cell_values/Interval.hpp"

Interval::Interval(int numerator_input, int denominator_input, int octave_input)
    : numerator(numerator_input), denominator(denominator_input),
      octave(octave_input) {}

auto Interval::operator==(const Interval &other_interval) const -> bool {
  return numerator == other_interval.numerator &&
         denominator == other_interval.denominator &&
         octave == other_interval.octave;
}
