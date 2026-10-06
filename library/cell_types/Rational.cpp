#include "cell_types/Rational.hpp"

#include <QtCore/QTextStream>

#include "other/helpers.hpp"
#include "xml/XMLChildren.hpp"

Rational::Rational(const int numerator_input, const int denominator_input) {
  Q_ASSERT(numerator_input != 0);
  Q_ASSERT(denominator_input != 0);
  const auto common_denominator = std::gcd(numerator_input, denominator_input);
  numerator = numerator_input / common_denominator;
  denominator = denominator_input / common_denominator;
}

auto Rational::operator*(const Rational& other_rational) const -> Rational {
  return Rational(numerator * other_rational.numerator,
                  denominator * other_rational.denominator);
}

auto Rational::operator/(const Rational& other_rational) const -> Rational {
  Q_ASSERT(other_rational.numerator != 0);
  return Rational(numerator * other_rational.denominator,
                  denominator * other_rational.numerator);
}

auto rational_to_double(const Rational& rational) -> double {
  const auto denominator = rational.denominator;
  Q_ASSERT(denominator != 0);
  return (1.0 * rational.numerator) / denominator;
}

auto rational_to_qstring(const Rational& rational) -> QString {
  QString result;
  QTextStream stream(&result);
  if (rational.numerator != 1) {
    stream << rational.numerator;
  }
  if (rational.denominator != 1) {
    stream << "/" << rational.denominator;
  }
  return result;
}

void set_rational_from_xml(Rational& rational, xmlNode& node) {
  auto numerator = rational.numerator;
  auto denominator = rational.denominator;
  for (auto& field_node : get_xml_children(node)) {
    const auto& name = get_xml_name(field_node);
    if (name == "numerator") {
      numerator = xml_to_int(field_node);
    } else {
      Q_ASSERT(name == "denominator");
      denominator = xml_to_int(field_node);
    }
  }
  // the schema only bounds numerator/denominator to [1, 999], not that
  // they're coprime, so route through the reducing constructor to match the
  // canonical form every other Rational comes in (operator== assumes
  // reduced form)
  rational = Rational(numerator, denominator);
}

void maybe_add_rational_to_xml(xmlNode& node, const char* const column_name,
                               const Rational& rational) {
  if (rational != Rational()) {
    auto& rational_node = get_new_child(node, column_name);
    maybe_add_int_to_xml(rational_node, "numerator", rational.numerator, 1);
    maybe_add_int_to_xml(rational_node, "denominator", rational.denominator, 1);
  }
}
