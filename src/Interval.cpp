#include "Interval.h"

#include <qregularexpression.h>  // for QRegularExpressionMatch, QRegularExp...

#include <cmath>  // for pow

#include "Utilities.h"

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

void Interval::set_text(const QString &interval_text) {
  auto interval_match = INTERVAL_PATTERN.match(interval_text);
  numerator = get_capture_int(interval_match, "numerator", DEFAULT_NUMERATOR);
  denominator =
      get_capture_int(interval_match, "denominator", DEFAULT_DENOMINATOR);
  octave = get_capture_int(interval_match, "octave", DEFAULT_OCTAVE);
}

auto Interval::verify_json(const QString &interval_text) -> bool {
  auto interval_match = INTERVAL_PATTERN.match(interval_text);
  if (!(INTERVAL_PATTERN.match(interval_text).hasMatch())) {
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
