#pragma once

#include <QByteArray>
#include <QMetaType>
#include <nlohmann/json.hpp>

struct Interval {
  int numerator = 1;
  int denominator = 1;
  int octave = 0;
  [[nodiscard]] auto operator==(const Interval &other_interval) const -> bool;
};

Q_DECLARE_METATYPE(Interval);

[[nodiscard]] auto interval_to_double(const Interval &interval) -> double;

[[nodiscard]] auto interval_is_default(const Interval &interval) -> bool;
[[nodiscard]] auto
json_to_interval(const nlohmann::json &json_interval) -> Interval;
[[nodiscard]] auto interval_to_json(const Interval &interval) -> nlohmann::json;