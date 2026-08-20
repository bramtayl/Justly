#pragma once

#include <libxml/parser.h>

#include <QtCore/QMetaType>

static const auto MAX_NUMERATOR = 999;
static const auto MAX_DENOMINATOR = 999;

struct Rational {
  int numerator;
  int denominator;

  explicit Rational(int numerator_input = 1, int denominator_input = 1);

  [[nodiscard]] auto operator*(const Rational& other_interval) const
      -> Rational;

  [[nodiscard]] auto operator/(const Rational& other_interval) const
      -> Rational;

  [[nodiscard]] auto operator==(const Rational& other_rational) const -> bool;
};

Q_DECLARE_METATYPE(Rational);

[[nodiscard]] auto rational_to_double(const Rational& rational) -> double;

[[nodiscard]] auto rational_is_default(const Rational& rational) -> bool;

void set_rational_from_xml(Rational& rational, xmlNode& node);

void maybe_add_int_to_xml(xmlNode& node, const char* field_name, int value,
                          int default_value);

void maybe_add_rational_to_xml(xmlNode& node, const char* column_name,
                               const Rational& rational);
