#include "cell_types/Interval.hpp"

#include "other/helpers.hpp"

Interval::Interval(Rational ratio_input, const int octave_input)
    : ratio(ratio_input), octave(octave_input) {
  // a 0 numerator would never become odd by halving
  Q_ASSERT(ratio.numerator != 0);
  Q_ASSERT(ratio.denominator != 0);
  while (ratio.numerator % 2 == 0) {
    ratio.numerator = ratio.numerator / 2;
    octave = octave + 1;
  }
  while (ratio.denominator % 2 == 0) {
    ratio.denominator = ratio.denominator / 2;
    octave = octave - 1;
  }
}

auto get_just_scale() -> const QList<NamedRatio>& {
  static const QList<NamedRatio> scale = {
      {.name = "Unison", .ratio = Rational(1, 1)},
      {.name = "Minor second", .ratio = Rational(16, 15)},
      {.name = "Major second", .ratio = Rational(9, 8)},
      {.name = "Minor third", .ratio = Rational(6, 5)},
      {.name = "Major third", .ratio = Rational(5, 4)},
      {.name = "Perfect fourth", .ratio = Rational(4, 3)},
      {.name = "Tritone", .ratio = Rational(45, 32)},
      {.name = "Perfect fifth", .ratio = Rational(3, 2)},
      {.name = "Minor sixth", .ratio = Rational(8, 5)},
      {.name = "Major sixth", .ratio = Rational(5, 3)},
      {.name = "Minor seventh", .ratio = Rational(9, 5)},
      {.name = "Major seventh", .ratio = Rational(15, 8)}};
  return scale;
}

auto Interval::operator==(const Interval& other_interval) const -> bool {
  return ratio == other_interval.ratio && octave == other_interval.octave;
}

auto Interval::operator*(const Interval& other_interval) const -> Interval {
  return Interval(ratio * other_interval.ratio, octave + other_interval.octave);
}

auto Interval::operator/(const Interval& other_interval) const -> Interval {
  return Interval(ratio / other_interval.ratio, octave - other_interval.octave);
}

auto interval_to_double(const Interval& interval) -> double {
  return rational_to_double(interval.ratio) *
         pow(OCTAVE_RATIO, interval.octave);
}

void set_interval_from_xml(Interval& interval, xmlNode& node) {
  auto* field_pointer = xmlFirstElementChild(&node);
  while (field_pointer != nullptr) {
    auto& field_node = get_reference(field_pointer);
    const auto name = get_xml_name(field_node);
    if (name == "ratio") {
      set_rational_from_xml(interval.ratio, field_node);
    } else {
      Q_ASSERT(name == "octave");
      interval.octave = xml_to_int(field_node);
    }
    field_pointer = xmlNextElementSibling(field_pointer);
  }
  // route through the normalizing constructor so an XML ratio with even
  // numerator/denominator factors folds into octave, matching the canonical
  // form every other Interval comes in (see set_rational_from_xml)
  interval = Interval(interval.ratio, interval.octave);
}

void maybe_add_interval_to_xml(xmlNode& node, const char* const column_name,
                               const Interval& interval) {
  const auto& ratio = interval.ratio;
  const auto octave = interval.octave;
  if (!rational_is_default(ratio) || octave != 0) {
    auto& interval_node = get_new_child(node, column_name);
    maybe_add_rational_to_xml(interval_node, "ratio", ratio);
    maybe_add_int_to_xml(interval_node, "octave", octave, 0);
  }
}
