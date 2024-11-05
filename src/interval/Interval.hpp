#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QVariant>
#include <nlohmann/json.hpp>

#include "rational/AbstractRational.hpp"

const auto MAX_OCTAVE = 9;

struct Interval : public AbstractRational {
  int octave = 0;

  Interval() = default;
  Interval(int numerator, int denominator, int octave_octave_input);
  explicit Interval(const nlohmann::json &json_rational);

  [[nodiscard]] auto operator==(const Interval &other_interval) const -> bool;
  [[nodiscard]] auto is_default() const -> bool override;
  [[nodiscard]] auto to_double() const -> double override;
  [[nodiscard]] auto to_json() const -> nlohmann::json override;
};

Q_DECLARE_METATYPE(Interval);

[[nodiscard]] auto variant_to_interval(const QVariant &variant) -> Interval;

[[nodiscard]] auto get_interval_schema() -> nlohmann::json;

[[nodiscard]] auto
json_field_to_interval(const nlohmann::json &json_row) -> Interval;
