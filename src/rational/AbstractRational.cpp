#include "rational/AbstractRational.hpp"

#include <QtGlobal>
#include <nlohmann/json.hpp>

#include "other/other.hpp"

AbstractRational::AbstractRational(int numerator_input, int denominator_input)
    : numerator(numerator_input), denominator(denominator_input){

                                  };

AbstractRational::AbstractRational(const nlohmann::json &json_rational)
    : numerator(json_rational.value("numerator", 1)),
      denominator(json_rational.value("denominator", 1)) {}

auto AbstractRational::is_default() const -> bool {
  return numerator == 1 && denominator == 1;
}

auto AbstractRational::to_double() const -> double {
  Q_ASSERT(denominator != 0);
  return 1.0 * numerator / denominator;
}

auto AbstractRational::to_json() const -> nlohmann::json {
  nlohmann::json json_interval;
  add_int_to_json(json_interval, "numerator", numerator, 1);
  add_int_to_json(json_interval, "denominator", denominator, 1);
  return json_interval;
};

void add_abstract_rational_to_json(nlohmann::json &json_row,
                                   const AbstractRational &rational,
                                   const char *column_name) {
  if (!rational.is_default()) {
    json_row[column_name] = rational.to_json();
  }
}
