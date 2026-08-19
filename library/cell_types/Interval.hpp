#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>
#include <QtCore/QtAssert>
#include <cmath>
#include <libxml/parser.h>

#include "cell_types/Rational.hpp"
#include "other/helpers.hpp"

static const auto OCTAVE_RATIO = 2.0;

struct Interval {
  Rational ratio;
  int octave;

  explicit Interval(Rational ratio_input = Rational(1, 1),
                    const int octave_input = 0);

  [[nodiscard]] auto operator==(const Interval &other_interval) const -> bool;

  [[nodiscard]] auto operator*(const Interval &other_interval) const -> Interval;

  [[nodiscard]] auto operator/(const Interval &other_interval) const -> Interval;
};

Q_DECLARE_METATYPE(Interval);

[[nodiscard]] auto interval_to_double(const Interval &interval) -> double;

void set_interval_from_xml(Interval &interval, xmlNode &node);

void maybe_add_interval_to_xml(xmlNode &node,
                               const char *const column_name,
                               const Interval &interval);
