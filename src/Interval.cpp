#include "Interval.h"

#include <qregularexpression.h>  // for QRegularExpressionMatch, QRegularExp...

#include <cmath>  // for pow

#include "utilities.h"

Interval::Interval(int numerator_input, int denominator_input, int octave_input)
    : numerator(numerator_input),
      denominator(denominator_input),
      octave(octave_input){

      };

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

auto Interval::verify_json(const QString& interval_text) -> bool {
  auto interval_match = get_pattern().match(interval_text);
  if (!(interval_match.hasMatch())) {
    json_parse_error(QString("Non-interval %1!").arg(interval_text));
    return false;
  };
  if (!(verify_regex_int(interval_match, "numerator", MINIMUM_NUMERATOR,
                         MAXIMUM_NUMERATOR))) {
    return false;
  }
  if (!(verify_regex_int(interval_match, "denominator", MINIMUM_DENOMINATOR,
                         MAXIMUM_DENOMINATOR))) {
    return false;
  }
  if (!(verify_regex_int(interval_match, "octave", MINIMUM_OCTAVE,
                         MAXIMUM_OCTAVE))) {
    return false;
  }
  return true;
}

auto Interval::is_default() const -> bool {
  return numerator == DEFAULT_NUMERATOR && denominator == DEFAULT_DENOMINATOR &&
         octave == DEFAULT_OCTAVE;
}

auto Interval::get_ratio() const -> double {
  return (1.0 * numerator) / denominator * pow(OCTAVE_RATIO, octave);
}

auto Interval::interval_from_text(const QString& interval_text) -> Interval {
  auto interval_match = get_pattern().match(interval_text);
  return Interval(
      get_capture_int(interval_match, "numerator", DEFAULT_NUMERATOR),
      get_capture_int(interval_match, "denominator", DEFAULT_DENOMINATOR),
      get_capture_int(interval_match, "octave", DEFAULT_OCTAVE));
}

auto Interval::operator==(const Interval& other_interval) const -> bool {
  return numerator == other_interval.numerator &&
         denominator == other_interval.denominator &&
         octave == other_interval.octave;
}

auto Interval::get_pattern() -> QRegularExpression& {
  static auto interval_pattern = QRegularExpression(
      R"((?<numerator>\d+)(\/(?<denominator>\d+))?(o(?<octave>-?\d+))?)");
  return interval_pattern;
}