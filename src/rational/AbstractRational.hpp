#pragma once

#include <nlohmann/json.hpp>

const auto MAX_RATIONAL_NUMERATOR = 199;
const auto MAX_RATIONAL_DENOMINATOR = 199;

struct AbstractRational {
  int numerator = 1;
  int denominator = 1;

  AbstractRational() = default;
  AbstractRational(int numerator_input, int denominator_input);
  explicit AbstractRational(const nlohmann::json &json_rational);
  virtual ~AbstractRational() = default;

  [[nodiscard]] virtual auto is_default() const -> bool;
  [[nodiscard]] virtual auto to_double() const -> double;
  [[nodiscard]] virtual auto to_json() const -> nlohmann::json;
};

void add_abstract_rational_to_json(nlohmann::json &json_row,
                                   const AbstractRational &rational,
                                   const char *column_name);
