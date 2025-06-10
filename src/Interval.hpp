#pragma once

#include "Rational.hpp"

struct Interval {
  Rational ratio;
  int octave;

  explicit Interval(Rational ratio_input = Rational(1, 1),
                    const int octave_input = 0)
      : ratio(ratio_input), octave(octave_input) {
    while (ratio.numerator % 2 == 0) {
      ratio.numerator = ratio.numerator / 2;
      octave = octave + 1;
    }
    while (ratio.denominator % 2 == 0) {
      ratio.denominator = ratio.denominator / 2;
      octave = octave - 1;
    }
  }

  [[nodiscard]] auto operator==(const Interval &other_interval) const {
    return ratio == other_interval.ratio && octave == other_interval.octave;
  }

  [[nodiscard]] auto operator*(const Interval &other_interval) const {
    return Interval(ratio * other_interval.ratio,
                    octave + other_interval.octave);
  }

  [[nodiscard]] auto operator/(const Interval &other_interval) const {
    return Interval(ratio / other_interval.ratio,
                    octave - other_interval.octave);
  }
};

Q_DECLARE_METATYPE(Interval);

[[nodiscard]] static inline auto interval_to_double(const Interval &interval) {
  return rational_to_double(interval.ratio) *
         pow(OCTAVE_RATIO, interval.octave);
}

static inline void xml_to_interval(Interval &interval, xmlNode &node) {
  auto *field_pointer = xmlFirstElementChild(&node);
  while ((field_pointer != nullptr)) {
    auto &field_node = get_reference(field_pointer);
    const auto name = get_xml_name(field_node);
    if (name == "ratio") {
      maybe_xml_to_rational(interval.ratio, field_node);
    } else if (name == "octave") {
      interval.octave = xml_to_int(field_node);
    } else {
      Q_ASSERT(false);
    }
    field_pointer = xmlNextElementSibling(field_pointer);
  }
}

static inline void maybe_set_xml_interval(xmlNode &node, const char *const column_name,
                                   const Interval &interval) {
  const auto &ratio = interval.ratio;
  const auto octave = interval.octave;
  if (!rational_is_default(ratio) || octave != 0) {
    auto &interval_node = get_new_child(node, column_name);
    maybe_set_xml_rational(interval_node, "ratio", ratio);
    maybe_set_xml_int(interval_node, "octave", octave, 0);
  }
}
