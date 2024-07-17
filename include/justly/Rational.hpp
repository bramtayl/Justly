#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <nlohmann/json.hpp>
#include <string>

#include "justly/public_constants.hpp"

struct JUSTLY_EXPORT Rational {
  int numerator;
  int denominator;

  explicit Rational(int numerator = 1, int denominator = 1);
  explicit Rational(const nlohmann::json &json_rational);

  [[nodiscard]] auto operator==(const Rational &other_rational) const -> bool;

  [[nodiscard]] auto is_default() const -> bool;

  [[nodiscard]] auto ratio() const -> double;
  [[nodiscard]] auto text() const -> QString;
  [[nodiscard]] auto json() const -> nlohmann::json;
};

Q_DECLARE_METATYPE(Rational);

[[nodiscard]] auto
get_rational_schema(const std::string &description) -> nlohmann::json &;

[[nodiscard]] auto get_interval_schema() -> const nlohmann::json &;
