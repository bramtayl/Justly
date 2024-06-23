#pragma once

#include <qmetatype.h>  // for qRegisterMetaType, qRegisterNormaliz...

#include <nlohmann/json_fwd.hpp>  // for json
#include <string>

#include "justly/constants.hpp"
#include "justly/global.hpp"

struct JUSTLY_EXPORT Rational {
  int numerator;
  int denominator;

  explicit Rational(int numerator = DEFAULT_NUMERATOR,
                    int denominator = DEFAULT_DENOMINATOR);
  explicit Rational(const nlohmann::json &json_rational);

  [[nodiscard]] auto operator==(const Rational &other_rational) const -> bool;

  [[nodiscard]] auto is_default() const -> bool;

  [[nodiscard]] auto ratio() const -> double;
  [[nodiscard]] auto text() const -> std::string;
  [[nodiscard]] auto json() const -> nlohmann::json;
};

Q_DECLARE_METATYPE(Rational);