#pragma once

#include <concepts>
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

  [[nodiscard]] auto
  operator==(const AbstractRational &other_rational) const -> bool;

  [[nodiscard]] virtual auto is_default() const -> bool;
  [[nodiscard]] virtual auto to_double() const -> double;
  virtual void to_json(nlohmann::json &json_rational) const;
};

[[nodiscard]] auto get_numerator_schema() -> nlohmann::json;

[[nodiscard]] auto get_denominator_schema() -> nlohmann::json;

void add_abstract_rational_to_json(nlohmann::json &json_row,
                                   const AbstractRational &rational,
                                   const char *column_name);

template <std::derived_from<AbstractRational> SubRational>
auto json_field_to_abstract_rational(const nlohmann::json &json_row,
                                     const char *field_name) -> SubRational {
  if (json_row.contains(field_name)) {
    return SubRational(json_row[field_name]);
  }
  return {};
}
