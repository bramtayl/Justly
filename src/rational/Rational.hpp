#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QVariant>
#include <nlohmann/json.hpp>

const auto MAX_RATIONAL_NUMERATOR = 199;
const auto MAX_RATIONAL_DENOMINATOR = 199;

struct Rational {
  int numerator = 1;
  int denominator = 1;

  [[nodiscard]] auto operator==(const Rational &other_rational) const -> bool;
};

Q_DECLARE_METATYPE(Rational);

[[nodiscard]] auto variant_to_rational(const QVariant &variant) -> Rational;

[[nodiscard]] auto rational_is_default(const Rational &rational) -> bool;
[[nodiscard]] auto rational_to_double(const Rational &rational) -> double;

[[nodiscard]] auto
get_rational_schema(const char *description) -> nlohmann::json;
[[nodiscard]] auto rational_to_json(const Rational &rational) -> nlohmann::json;
[[nodiscard]] auto
json_to_rational(const nlohmann::json &json_rational) -> Rational;
