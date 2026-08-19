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
                    const int denominator_input = 1);

  [[nodiscard]] auto operator*(const Rational &other_interval) const -> Rational;

  [[nodiscard]] auto operator/(const Rational &other_interval) const -> Rational;

  [[nodiscard]] auto operator==(const Rational &other_rational) const -> bool;
};

Q_DECLARE_METATYPE(Rational);

[[nodiscard]] auto rational_to_double(const Rational &rational) -> double;

[[nodiscard]] auto rational_is_default(const Rational &rational) -> bool;

void set_rational_from_xml(Rational &rational, xmlNode &node);

void maybe_add_int_to_xml(xmlNode &node, const char *const field_name,
                          const int value, const int default_value);

void maybe_add_rational_to_xml(xmlNode &node,
                               const char *const column_name,
                               const Rational &rational);
