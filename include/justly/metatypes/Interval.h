#pragma once

#include <nlohmann/json_fwd.hpp>  // for json
#include <string>

const auto MINIMUM_NUMERATOR = 1;
const auto DEFAULT_NUMERATOR = 1;
const auto MAXIMUM_NUMERATOR = 199;

const auto MINIMUM_DENOMINATOR = 1;
const auto DEFAULT_DENOMINATOR = 1;
const auto MAXIMUM_DENOMINATOR = 199;

const auto MINIMUM_OCTAVE = -9;
const auto DEFAULT_OCTAVE = 0;
const auto MAXIMUM_OCTAVE = 9;

class Interval {
 public:
  int numerator;
  int denominator;
  int octave;
  explicit Interval(int numerator = DEFAULT_NUMERATOR,
                    int denominator = DEFAULT_DENOMINATOR,
                    int octave = DEFAULT_OCTAVE);
  explicit Interval(const nlohmann::json &json_interval);
  [[nodiscard]] auto get_text() const -> std::string;
  [[nodiscard]] static auto get_schema() -> const nlohmann::json &;
  [[nodiscard]] auto is_default() const -> bool;
  [[nodiscard]] auto get_ratio() const -> double;
  [[nodiscard]] auto operator==(const Interval &other_interval) const -> bool;
  [[nodiscard]] auto to_json() const -> nlohmann::json;
};
