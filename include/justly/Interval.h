#pragma once

#include <nlohmann/json_fwd.hpp>  // for json
#include <string>

#include <qmetatype.h>     // for qRegisterMetaType, qRegisterNormaliz...

#include "justly/global.h"

const auto MIN_NUMERATOR = 1;
const auto DEFAULT_NUMERATOR = 1;
const auto MAX_NUMERATOR = 199;

const auto MIN_DENOMINATOR = 1;
const auto DEFAULT_DENOMINATOR = 1;
const auto MAX_DENOMINATOR = 199;

const auto MIN_OCTAVE = -9;
const auto DEFAULT_OCTAVE = 0;
const auto MAX_OCTAVE = 9;

struct JUSTLY_EXPORT Interval {
  int numerator;
  int denominator;
  int octave;

  explicit Interval(int numerator = DEFAULT_NUMERATOR,
                    int denominator = DEFAULT_DENOMINATOR,
                    int octave = DEFAULT_OCTAVE);
  explicit Interval(const nlohmann::json &json_interval);

  [[nodiscard]] auto operator==(const Interval &other_interval) const -> bool;

  [[nodiscard]] auto is_default() const -> bool;

  [[nodiscard]] auto ratio() const -> double;
  [[nodiscard]] auto text() const -> std::string;
  [[nodiscard]] auto json() const -> nlohmann::json;
};

Q_DECLARE_METATYPE(Interval);