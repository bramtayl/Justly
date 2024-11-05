#pragma once

#include <QByteArray>
#include <QMetaType>
#include <nlohmann/json.hpp>

#include "rational/AbstractRational.hpp"

struct Rational : public AbstractRational {
  Rational() = default;
  Rational(int numerator, int denominator);
  explicit Rational(const nlohmann::json &json_rational);

  [[nodiscard]] auto operator==(const Rational &other_rational) const -> bool;
};

Q_DECLARE_METATYPE(Rational);

[[nodiscard]] auto
get_rational_schema(const char *description) -> nlohmann::json;
