#include "cell_types/Interval.hpp"

#include "other/helpers.hpp"

Interval::Interval(Rational ratio_input, const int octave_input)
    : ratio(ratio_input), octave(octave_input) {
  // a numerator of 0 can never become odd by halving -- 0 % 2 == 0 forever
  // -- so skip the folding loop rather than hang; a 0 ratio isn't a valid
  // frequency ratio anyway, so there's nothing meaningful to fold
  if (ratio.numerator != 0) {
    while (ratio.numerator % 2 == 0) {
      ratio.numerator = ratio.numerator / 2;
      octave = octave + 1;
    }
  }
  while (ratio.denominator % 2 == 0) {
    ratio.denominator = ratio.denominator / 2;
    octave = octave - 1;
  }
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
    } else if (name == "octave") {
      interval.octave = xml_to_int(field_node);
    } else {
      Q_UNREACHABLE();
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
