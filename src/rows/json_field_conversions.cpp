#include "rows/json_field_conversions.hpp"

#include <nlohmann/json.hpp>
#include <utility>

#include "interval/Interval.hpp"
#include "rational/Rational.hpp"

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
    json_row["interval"] = json_interval;
  }
}

void add_rational_to_json(nlohmann::json &json_row, const Rational &rational,
                          const char *column_name) {
  if (rational.numerator != 1 || rational.denominator != 1) {
    auto numerator = rational.numerator;
    auto denominator = rational.denominator;

    auto json_rational = nlohmann::json::object();
    if (numerator != 1) {
      json_rational["numerator"] = numerator;
    }
    if (denominator != 1) {
      json_rational["denominator"] = denominator;
    }
    json_row[column_name] = std::move(json_rational);
  }
}

void add_words_to_json(nlohmann::json &json_row, const QString &words) {
  if (!words.isEmpty()) {
    json_row["words"] = words.toStdString().c_str();
  }
}
