#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QVariant>
#include <nlohmann/json.hpp>

const auto MAX_INTERVAL_NUMERATOR = 199;
const auto MAX_INTERVAL_DENOMINATOR = 199;

const auto MIN_OCTAVE = -9;
const auto MAX_OCTAVE = 9;

struct Interval {
  int numerator = 1;
  int denominator = 1;
  int octave = 0;
  [[nodiscard]] auto operator==(const Interval &other_interval) const -> bool;
};

Q_DECLARE_METATYPE(Interval);

[[nodiscard]] auto variant_to_interval(const QVariant &variant) -> Interval;

[[nodiscard]] auto interval_to_double(const Interval &interval) -> double;

[[nodiscard]] auto get_interval_schema() -> nlohmann::json;

void add_interval_to_json(nlohmann::json &json_row, const Interval &interval);

[[nodiscard]] auto json_field_to_interval(const nlohmann::json &json_row) -> Interval;
