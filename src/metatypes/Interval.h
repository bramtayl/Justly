#pragma once

#include <qmetatype.h>  // for qRegisterMetaType, qRegisterNormaliz...
#include <qstring.h>    // for QString

#include <nlohmann/json_fwd.hpp>  // for json

const auto MINIMUM_NUMERATOR = 1;
const auto DEFAULT_NUMERATOR = 1;
const auto MAXIMUM_NUMERATOR = 199;

const auto MINIMUM_DENOMINATOR = 1;
const auto DEFAULT_DENOMINATOR = 1;
const auto MAXIMUM_DENOMINATOR = 199;

const auto MINIMUM_OCTAVE = -9;
const auto DEFAULT_OCTAVE = 0;
const auto MAXIMUM_OCTAVE = 9;

const auto OCTAVE_RATIO = 2.0;

class Interval {
 public:
  int numerator;
  int denominator;
  int octave;
  explicit Interval(int numerator = DEFAULT_NUMERATOR,
                    int denominator = DEFAULT_DENOMINATOR,
                    int octave = DEFAULT_OCTAVE);
  [[nodiscard]] auto get_text() const -> QString;
  [[nodiscard]] static auto get_schema() -> const nlohmann::json &;
  [[nodiscard]] auto is_default() const -> bool;
  [[nodiscard]] auto get_ratio() const -> double;
  [[nodiscard]] auto operator==(const Interval &other_interval) const -> bool;
  auto save_to(nlohmann::json &json_map) const -> void;
  void load_from(const nlohmann::json &json_interval);
};

Q_DECLARE_METATYPE(Interval)
