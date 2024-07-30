#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <nlohmann/json.hpp>

#include "justly/JUSTLY_EXPORT.hpp"

const auto MAX_INTERVAL_NUMERATOR = 199;
const auto MAX_INTERVAL_DENOMINATOR = 199;

const auto MIN_OCTAVE = -9;
const auto MAX_OCTAVE = 9;

struct JUSTLY_EXPORT Interval {
  int numerator;
  int denominator;
  int octave;

  explicit Interval(int numerator = 1, int denominator = 1, int octave = 0);
  explicit Interval(const nlohmann::json &json_interval);

  [[nodiscard]] auto operator==(const Interval &other_interval) const -> bool;

  [[nodiscard]] auto is_default() const -> bool;

  [[nodiscard]] auto ratio() const -> double;
  [[nodiscard]] auto text() const -> QString;
  [[nodiscard]] auto to_json() const -> nlohmann::json;
};

Q_DECLARE_METATYPE(Interval);
