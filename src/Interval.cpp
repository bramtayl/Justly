#include "Interval.h"

#include <qjsonvalue.h>          // for QJsonValue, QJsonValueRef
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

auto Interval::is_default() const -> bool {
  return numerator == DEFAULT_NUMERATOR && denominator == DEFAULT_DENOMINATOR &&
         octave == DEFAULT_OCTAVE;
}

auto Interval::get_ratio() const -> double {
  return (1.0 * numerator) / denominator * pow(OCTAVE_RATIO, octave);
}

auto Interval::parse_interval(const QString& interval_text) -> Interval {
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

auto Interval::get_schema() -> QString& {
  static auto interval_schema = QString(R"(
  {
      "type": "object",
      "description": "an interval",
      "properties": {
          "numerator": {
              "type": "integer",
              "description": "the numerator",
              "minimum": %1,
              "maximum": %2
          },
          "denominator": {
              "type": "integer",
              "description": "the denominator",
              "minimum": %3,
              "maximum": %4
          },
          "octave": {
              "type": "integer",
              "description": "the octave",
              "minimum": %5,
              "maximum": %6
          }
      }
  })")
  .arg(MINIMUM_NUMERATOR)
  .arg(MAXIMUM_NUMERATOR)
  .arg(MINIMUM_DENOMINATOR)
  .arg(MAXIMUM_DENOMINATOR)
  .arg(MINIMUM_OCTAVE)
  .arg(MAXIMUM_OCTAVE);
  return interval_schema;
}

auto Interval::get_pattern() -> QRegularExpression& {
  static auto interval_pattern = QRegularExpression(
      R"((?<numerator>\d+)(\/(?<denominator>\d+))?(o(?<octave>-?\d+))?)");
  return interval_pattern;
}

auto Interval::save_to(QJsonObject &json_map) const -> void {
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

void Interval::load_from(const QJsonObject &json_interval) {
  if (json_interval.contains("numerator")) {
    numerator = json_interval["numerator"].toInt();
  }
  if (json_interval.contains("denominator")) {
    denominator = json_interval["denominator"].toInt();
  }
  if (json_interval.contains("octave")) {
    octave = json_interval["octave"].toInt();
  }
}