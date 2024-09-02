#pragma once

#include <QByteArray>
#include <QMetaType>
#include <nlohmann/json.hpp>

struct Rational {
  int numerator = 1;
  int denominator = 1;

  [[nodiscard]] auto operator==(const Rational &other_rational) const -> bool;
};

Q_DECLARE_METATYPE(Rational);

auto rational_is_default(const Rational &rational) -> bool;
auto rational_to_json(const Rational &rational) -> nlohmann::json;
auto json_to_rational(const nlohmann::json &json_rational) -> Rational;
