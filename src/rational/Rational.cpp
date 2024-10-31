#include "rational/Rational.hpp"

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
  return nlohmann::json(
      {{"type", "object"},
       {"description", description},
       {"properties",
        nlohmann::json(
            {{"numerator",
              nlohmann::json({{"type", "integer"},
                              {"description", "the numerator"},
                              {"minimum", 1},
                              {"maximum", MAX_RATIONAL_NUMERATOR}})},
             {"denominator",
              nlohmann::json({{"type", "integer"},
                              {"description", "the denominator"},
                              {"minimum", 1},
                              {"maximum", MAX_RATIONAL_DENOMINATOR}})}})}});
}

void json_field_to_rational(Rational& rational, const nlohmann::json &json_object, const char* field_name) {
  if (json_object.contains(field_name)) {
    const auto& json_rational = json_object[field_name];
    rational.numerator = json_rational.value("numerator", 1);
    rational.denominator = json_rational.value("denominator", 1);
  }
}

void add_rational_to_json(nlohmann::json &json_row, const Rational &rational,
                          const char *column_name) {
  if (rational.numerator != 1 || rational.denominator != 1) {
    auto numerator = rational.numerator;
    auto denominator = rational.denominator;

    auto json_rational = nlohmann::json::object();
    if (numerator != 1) {
      json_rational["numerator"] = numerator;
    }
    if (denominator != 1) {
      json_rational["denominator"] = denominator;
    }
    json_row[column_name] = std::move(json_rational);
  }
}

auto json_field_to_rational(const nlohmann::json &json_row, const char* field_name) -> Rational {
  if (json_row.contains(field_name)) {
    const auto &json_rational = json_row[field_name];
    return {json_rational.value("numerator", 1),
                             json_rational.value("denominator", 1)};
  }
  return {};
}