#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QVariant>
#include <nlohmann/json.hpp>

#include "rational/AbstractRational.hpp"

struct Rational : public AbstractRational {
  Rational() = default;
  Rational(int numerator, int denominator);
  explicit Rational(const nlohmann::json &json_rational);

  [[nodiscard]] auto operator==(const Rational &other_rational) const -> bool;
};

Q_DECLARE_METATYPE(Rational);

[[nodiscard]] auto variant_to_rational(const QVariant &variant) -> Rational;

[[nodiscard]] auto
get_rational_schema(const char *description) -> nlohmann::json;
void json_field_to_rational(Rational &rational,
                            const nlohmann::json &json_object,
                            const char *field_name);

[[nodiscard]] auto json_field_to_rational(const nlohmann::json &json_row,
                                          const char *field_name) -> Rational;
