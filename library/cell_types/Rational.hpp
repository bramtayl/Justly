#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>
#include <QtCore/QtAssert>
#include <libxml/parser.h>
#include <numeric>

#include "other/helpers.hpp"

static const auto MAX_NUMERATOR = 999;
static const auto MAX_DENOMINATOR = 999;

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

[[nodiscard]] static inline auto rational_to_double(const Rational &rational) {
  const auto denominator = rational.denominator;
  Q_ASSERT(denominator != 0);
  return (1.0 * rational.numerator) / denominator;
}

[[nodiscard]] static auto rational_is_default(const Rational &rational) {
  return rational.numerator == 1 && rational.denominator == 1;
}

static inline void set_rational_from_xml(Rational &rational, xmlNode &node) {
  auto numerator = rational.numerator;
  auto denominator = rational.denominator;
  auto *field_pointer = xmlFirstElementChild(&node);
  while (field_pointer != nullptr) {
    auto &field_node = get_reference(field_pointer);
    const auto &name = get_xml_name(field_node);
    if (name == "numerator") {
      numerator = xml_to_int(field_node);
    } else if (name == "denominator") {
      denominator = xml_to_int(field_node);
    } else {
      Q_ASSERT(false);
    }
    field_pointer = xmlNextElementSibling(field_pointer);
  }
  // the schema only bounds numerator/denominator to [1, 999], not that
  // they're coprime, so route through the reducing constructor to match the
  // canonical form every other Rational comes in (operator== and
  // rational_is_default both assume reduced form)
  rational = Rational(numerator, denominator);
}

static void maybe_add_int_to_xml(xmlNode &node,
                                     const char *const field_name,
                                     const int value, const int default_value) {
  if (value != default_value) {
    set_xml_int(node, field_name, value);
  }
}

static inline void maybe_add_rational_to_xml(xmlNode &node,
                                          const char *const column_name,
                                          const Rational &rational) {
  if (!rational_is_default(rational)) {
    auto &rational_node = get_new_child(node, column_name);
    maybe_add_int_to_xml(rational_node, "numerator", rational.numerator, 1);
    maybe_add_int_to_xml(rational_node, "denominator", rational.denominator, 1);
  }
}
