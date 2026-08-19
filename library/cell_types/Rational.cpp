#include "cell_types/Rational.hpp"

Rational::Rational(const int numerator_input, const int denominator_input) {
  Q_ASSERT(denominator_input != 0);
  const auto common_denominator =
      std::gcd(numerator_input, denominator_input);
  numerator = numerator_input / common_denominator;
  denominator = denominator_input / common_denominator;
}

auto Rational::operator*(const Rational &other_interval) const -> Rational {
  return Rational(numerator * other_interval.numerator,
                  denominator * other_interval.denominator);
}

auto Rational::operator/(const Rational &other_interval) const -> Rational {
  Q_ASSERT(other_interval.numerator != 0);
  return Rational(numerator * other_interval.denominator,
                  denominator * other_interval.numerator);
}

auto Rational::operator==(const Rational &other_rational) const -> bool {
  return numerator == other_rational.numerator &&
         denominator == other_rational.denominator;
}

auto rational_to_double(const Rational &rational) -> double {
  const auto denominator = rational.denominator;
  Q_ASSERT(denominator != 0);
  return (1.0 * rational.numerator) / denominator;
}

auto rational_is_default(const Rational &rational) -> bool {
  return rational.numerator == 1 && rational.denominator == 1;
}

void set_rational_from_xml(Rational &rational, xmlNode &node) {
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
      Q_UNREACHABLE();
    }
    field_pointer = xmlNextElementSibling(field_pointer);
  }
  // the schema only bounds numerator/denominator to [1, 999], not that
  // they're coprime, so route through the reducing constructor to match the
  // canonical form every other Rational comes in (operator== and
  // rational_is_default both assume reduced form)
  rational = Rational(numerator, denominator);
}

void maybe_add_int_to_xml(xmlNode &node, const char *const field_name,
                          const int value, const int default_value) {
  if (value != default_value) {
    set_xml_int(node, field_name, value);
  }
}

void maybe_add_rational_to_xml(xmlNode &node, const char *const column_name,
                               const Rational &rational) {
  if (!rational_is_default(rational)) {
    auto &rational_node = get_new_child(node, column_name);
    maybe_add_int_to_xml(rational_node, "numerator", rational.numerator, 1);
    maybe_add_int_to_xml(rational_node, "denominator", rational.denominator, 1);
  }
}
