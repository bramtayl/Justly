#pragma once

#include <qmetatype.h>  // for qRegisterMetaType, qRegisterNormaliz...

#include <nlohmann/json_fwd.hpp>  // for json
#include <string>

#include "justly/public_constants.hpp"

struct JUSTLY_EXPORT Interval {
  int numerator;
  int denominator;
  int octave;

  explicit Interval(int numerator = 1,
                    int denominator = 1,
                    int octave = 0);
  explicit Interval(const nlohmann::json &json_interval);

  [[nodiscard]] auto operator==(const Interval &other_interval) const -> bool;

  [[nodiscard]] auto is_default() const -> bool;

  [[nodiscard]] auto ratio() const -> double;
  [[nodiscard]] auto text() const -> std::string;
  [[nodiscard]] auto json() const -> nlohmann::json;
};

Q_DECLARE_METATYPE(Interval);

[[nodiscard]] auto get_interval_schema() -> const nlohmann::json&;