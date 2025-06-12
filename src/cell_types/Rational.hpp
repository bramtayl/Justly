#pragma once

#include "other/helpers.hpp"

static const auto MAX_NUMERATOR = 999;
static const auto MAX_DENOMINATOR = 999;

static const auto MILLISECONDS_PER_MINUTE = 60000;

struct Rational {
  int numerator;
  int denominator;

  explicit Rational(const int numerator_input = 1,
                    const int denominator_input = 1) {
    Q_ASSERT(denominator_input != 0);
    const auto common_denominator =
        std::gcd(numerator_input, denominator_input);
    numerator = numerator_input / common_denominator;
    denominator = denominator_input / common_denominator;
  }

  [[nodiscard]] auto operator*(const Rational &other_interval) const {
    return Rational(numerator * other_interval.numerator,
                    denominator * other_interval.denominator);
  }

  [[nodiscard]] auto operator/(const Rational &other_interval) const {
    return Rational(numerator * other_interval.denominator,
                    denominator * other_interval.numerator);
  }

  [[nodiscard]] auto operator==(const Rational &other_rational) const {
    return numerator == other_rational.numerator &&
           denominator == other_rational.denominator;
  }
};

Q_DECLARE_METATYPE(Rational);

[[nodiscard]] static auto rational_to_double(const Rational &rational) {
  const auto denominator = rational.denominator;
  Q_ASSERT(denominator != 0);
  return (1.0 * rational.numerator) / denominator;
}

[[nodiscard]] static auto rational_is_default(const Rational &rational) {
  return rational.numerator == 1 && rational.denominator == 1;
}

static inline void maybe_xml_to_rational(Rational &rational, xmlNode &node) {
  auto *field_pointer = xmlFirstElementChild(&node);
  while ((field_pointer != nullptr)) {
    auto &field_node = get_reference(field_pointer);
    const auto &name = get_xml_name(field_node);
    if (name == "numerator") {
      rational.numerator = xml_to_int(field_node);
    } else if (name == "denominator") {
      rational.denominator = xml_to_int(field_node);
    } else {
      Q_ASSERT(false);
    }
    field_pointer = xmlNextElementSibling(field_pointer);
  }
}

static inline void maybe_set_xml_int(xmlNode &node,
                                     const char *const field_name,
                                     const int value, const int default_value) {
  if (value != default_value) {
    set_xml_int(node, field_name, value);
  }
}

static inline void maybe_set_xml_rational(xmlNode &node, const char *const column_name,
                                   const Rational &rational) {
  if (!rational_is_default(rational)) {
    auto &rational_node = get_new_child(node, column_name);
    maybe_set_xml_int(rational_node, "numerator", rational.numerator, 1);
    maybe_set_xml_int(rational_node, "denominator", rational.denominator, 1);
  }
}

[[nodiscard]] static inline auto get_milliseconds(const double beats_per_minute,
                                           const Rational &beats) {
  return rational_to_double(beats) * MILLISECONDS_PER_MINUTE / beats_per_minute;
}
