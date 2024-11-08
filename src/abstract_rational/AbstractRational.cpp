#include "abstract_rational/AbstractRational.hpp"

#include <QtGlobal>
#include <nlohmann/json.hpp>
#include <utility>

AbstractRational::AbstractRational(int numerator_input, int denominator_input)
    : numerator(numerator_input), denominator(denominator_input){};

AbstractRational::AbstractRational(const nlohmann::json &json_rational)
    : numerator(json_rational.value("numerator", 1)),
      denominator(json_rational.value("denominator", 1)) {}

auto AbstractRational::operator==(const AbstractRational &other_rational) const -> bool {
  return numerator == other_rational.numerator &&
         denominator == other_rational.denominator;
}

auto AbstractRational::is_default() const -> bool {
  return numerator == 1 && denominator == 1;
}

auto AbstractRational::to_double() const -> double {
  Q_ASSERT(denominator != 0);
  return 1.0 * numerator / denominator;
}

void AbstractRational::to_json(nlohmann::json &json_rational) const {
  add_int_to_json(json_rational, "numerator", numerator, 1);
  add_int_to_json(json_rational, "denominator", denominator, 1);
}

void add_abstract_rational_to_json(nlohmann::json &json_row,
                                   const AbstractRational &rational,
                                   const char *column_name) {
  if (!rational.is_default()) {
    nlohmann::json json_rational;
    rational.to_json(json_rational);
    json_row[column_name] = std::move(json_rational);
  }
}

void add_int_to_json(nlohmann::json &json_object, const char *field_name,
                     int value, int default_value) {
  if (value != default_value) {
    json_object[field_name] = value;
  }
}