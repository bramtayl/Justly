#include "rational/Rational.hpp"
#include "other/other.hpp"

#include <QtGlobal>
#include <nlohmann/json.hpp>
#include <utility>

auto Rational::operator==(const Rational &other_rational) const -> bool {
  return numerator == other_rational.numerator &&
         denominator == other_rational.denominator;
}

auto variant_to_rational(const QVariant &variant) -> Rational {
  Q_ASSERT(variant.canConvert<Rational>());
  return variant.value<Rational>();
}

auto rational_to_double(const Rational &rational) -> double {
  Q_ASSERT(rational.denominator != 0);
  return 1.0 * rational.numerator / rational.denominator;
}

auto get_rational_schema(const char *description) -> nlohmann::json {
  return get_object_schema(
      description,
      nlohmann::json(
          {{"numerator", get_number_schema("integer", "numerator", 1,
                                           MAX_RATIONAL_NUMERATOR)},
           {"denominator", get_number_schema("integer", "denominator", 1,
                                             MAX_RATIONAL_DENOMINATOR)}}));
}

void add_rational_to_json(nlohmann::json &json_row, const Rational &rational,
                          const char *column_name) {
  auto numerator = rational.numerator;
  auto denominator = rational.denominator;
  if (numerator != 1 || denominator != 1) {
    nlohmann::json json_rational;
    add_int_to_json(json_rational, "numerator", numerator, 1);
    add_int_to_json(json_rational, "denominator", denominator, 1);
    json_row[column_name] = std::move(json_rational);
  }
}

auto json_field_to_rational(const nlohmann::json &json_row,
                            const char *field_name) -> Rational {
  if (json_row.contains(field_name)) {
    const auto &json_rational = json_row[field_name];
    return {json_rational.value("numerator", 1),
            json_rational.value("denominator", 1)};
  }
  return {};
}