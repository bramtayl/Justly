#pragma once

#include <nlohmann/json_fwd.hpp>  // for json
#include <string>

#include "justly/global.h"

JUSTLY_EXPORT const auto MIN_NUMERATOR = 1;
JUSTLY_EXPORT const auto DEFAULT_NUMERATOR = 1;
JUSTLY_EXPORT const auto MAX_NUMERATOR = 199;

JUSTLY_EXPORT const auto MIN_DENOMINATOR = 1;
JUSTLY_EXPORT const auto DEFAULT_DENOMINATOR = 1;
JUSTLY_EXPORT const auto MAX_DENOMINATOR = 199;

JUSTLY_EXPORT const auto MIN_OCTAVE = -9;
JUSTLY_EXPORT const auto DEFAULT_OCTAVE = 0;
JUSTLY_EXPORT const auto MAX_OCTAVE = 9;

struct JUSTLY_EXPORT Interval {
  int numerator;
  int denominator;
  int octave;
  explicit Interval(int numerator = DEFAULT_NUMERATOR,
                    int denominator = DEFAULT_DENOMINATOR,
                    int octave = DEFAULT_OCTAVE);
  explicit Interval(const nlohmann::json &json_interval);
  [[nodiscard]] auto text() const -> std::string;
  [[nodiscard]] auto is_default() const -> bool;
  [[nodiscard]] auto ratio() const -> double;
  [[nodiscard]] auto operator==(const Interval &other_interval) const -> bool;
  [[nodiscard]] auto json() const -> nlohmann::json;
};
