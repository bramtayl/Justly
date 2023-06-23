#include "Interval.h"
#include "Utilities.h"

#include <algorithm>             // for all_of
#include <cmath>                // for pow
#include <qjsonvalue.h>         // for QJsonValueRef
#include <qlist.h>              // for QList, QList<>::iterator
#include <qregularexpression.h> // for QRegularExpressionMatch, QRegularExp...

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
  QRegularExpression const interval_pattern(
      R"((?<numerator>\d+)(\/(?<denominator>\d+))?(o(?<octave>-?\d+))?)");
  QRegularExpressionMatch interval_match =
      interval_pattern.match(interval_text);
  numerator = interval_match.captured("numerator").toInt();
  auto denominator_text = interval_match.captured("denominator");
  if (!(denominator_text.isNull())) {
    denominator = denominator_text.toInt();
  }
  auto octave_text = interval_match.captured("octave");
  if (!(octave_text.isNull())) {
    octave = octave_text.toInt();
  }
}

auto Interval::save(QJsonObject &json_map) const -> void {
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

void Interval::load(const QJsonObject &json_interval) {
  numerator = get_json_int(json_interval, "numerator", DEFAULT_NUMERATOR);
  denominator = get_json_int(json_interval, "denominator", DEFAULT_DENOMINATOR);
  octave = get_json_int(json_interval, "octave", DEFAULT_OCTAVE);
}

auto Interval::verify_json(const QJsonObject &json_interval) -> bool {
  auto keys = json_interval.keys();
  return std::all_of(
      keys.cbegin(), keys.cend(), [&json_interval](const auto &field_name) {
        if (field_name == "numerator") {
          if (!(verify_bounded_int(json_interval, field_name, MINIMUM_NUMERATOR,
                                   MAXIMUM_NUMERATOR))) {
            return false;
          }
        } else if (field_name == "denominator") {
          if (!(verify_bounded_int(json_interval, field_name,
                                   MINIMUM_DENOMINATOR, MAXIMUM_DENOMINATOR))) {
            return false;
          }
        } else if (field_name == "octave") {
          if (!(verify_bounded_int(json_interval, field_name, MINIMUM_OCTAVE,
                                   MAXIMUM_OCTAVE))) {
            return false;
          }
        } else {
          warn_unrecognized_field("interval", field_name);
          return false;
        }
        return true;
      });
}

auto Interval::is_default() const -> bool {
  return numerator == DEFAULT_NUMERATOR && denominator == DEFAULT_DENOMINATOR &&
         octave == DEFAULT_OCTAVE;
}

auto Interval::get_ratio() const -> double {
  return (1.0 * numerator) / denominator * pow(OCTAVE_RATIO, octave);
}
