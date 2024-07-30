#include "justly/Interval.hpp"

#include <QTextStream>
#include <QtGlobal>
#include <cmath>
#include <nlohmann/json.hpp>

const auto OCTAVE_RATIO = 2.0;

Interval::Interval(int numerator_input, int denominator_input, int octave_input)
    : numerator(numerator_input), denominator(denominator_input),
      octave(octave_input) {}

Interval::Interval(const nlohmann::json &json_interval)
    : numerator(json_interval.value("numerator", 1)),
      denominator(json_interval.value("denominator", 1)),
      octave(json_interval.value("octave", 0)) {
}

auto Interval::operator==(const Interval &other_interval) const -> bool {
  return numerator == other_interval.numerator &&
         denominator == other_interval.denominator &&
         octave == other_interval.octave;
}

auto Interval::is_default() const -> bool {
  return numerator == 1 && denominator == 1 && octave == 0;
}

auto Interval::ratio() const -> double {
  Q_ASSERT(denominator != 0);
  return (1.0 * numerator) / denominator * pow(OCTAVE_RATIO, octave);
}

auto Interval::text() const -> QString {
  QString result;
  QTextStream stream(&result);
  if (numerator != 1) {
    stream << numerator;
  }
  if (denominator != 1) {
    stream << "/" << denominator;
  }
  if (octave != 0) {
    stream << "o" << octave;
  }
  return result;
}

auto Interval::to_json() const -> nlohmann::json {
  auto json_interval = nlohmann::json::object();
  if (numerator != 1) {
    json_interval["numerator"] = numerator;
  }
  if (denominator != 1) {
    json_interval["denominator"] = denominator;
  }
  if (octave != 0) {
    json_interval["octave"] = octave;
  }
  return json_interval;
}
