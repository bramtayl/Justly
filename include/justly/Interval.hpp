#pragma once

#include <QByteArray>
#include <QMetaType>

#include "justly/JUSTLY_EXPORT.hpp"

struct JUSTLY_EXPORT Interval {
  int numerator;
  int denominator;
  int octave;

  explicit Interval(int numerator = 1, int denominator = 1, int octave = 0);
  [[nodiscard]] auto operator==(const Interval &other_interval) const -> bool;
};

Q_DECLARE_METATYPE(Interval);
