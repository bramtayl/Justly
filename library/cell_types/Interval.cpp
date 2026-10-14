#include "cell_types/Interval.hpp"

#include <QtWidgets/QMessageBox>

#include "common/helpers.hpp"
#include "io/XMLChildren.hpp"

Interval::Interval(Rational ratio_input, const int octave_input)
    : ratio(ratio_input), octave(octave_input) {
  // a 0 numerator would never become odd by halving
  Q_ASSERT(ratio.numerator != 0);
  Q_ASSERT(ratio.denominator != 0);
  while (ratio.numerator % 2 == 0) {
    ratio.numerator = ratio.numerator / 2;
    ++octave;
  }
  while (ratio.denominator % 2 == 0) {
    ratio.denominator = ratio.denominator / 2;
    --octave;
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

auto interval_to_qstring(const Interval& interval) -> QString {
  auto result = rational_to_qstring(interval.ratio);
  if (interval.octave != 0) {
    result += "o" + QString::number(interval.octave);
  }
  return result;
}

void set_interval_from_xml(Interval& interval, xmlNode& node) {
  for (auto& field_node : get_xml_children(node)) {
    const auto name = get_xml_name(field_node);
    if (name == "ratio") {
      set_rational_from_xml(interval.ratio, field_node);
    } else {
      Q_ASSERT(name == "octave");
      interval.octave = xml_to_int(field_node);
    }
  }
  // route through the normalizing constructor so an XML ratio with even
  // numerator/denominator factors folds into octave, matching the canonical
  // form every other Interval comes in (see set_rational_from_xml)
  interval = Interval(interval.ratio, interval.octave);
}

void maybe_add_interval_to_xml(xmlNode& node, const char* const column_name,
                               const Interval& interval) {
  if (interval != Interval()) {
    auto& interval_node = get_new_child(node, column_name);
    maybe_add_rational_to_xml(interval_node, "ratio", interval.ratio);
    maybe_add_int_to_xml(interval_node, "octave", interval.octave, 0);
  }
}

namespace {

struct IntervalLimit {
  const char* name;
  int value;
  int maximum;
  const char* comparison;
};

}  // namespace

auto check_interval(QWidget& parent_widget, const Interval& interval) -> bool {
  for (const auto& limit :
       {IntervalLimit{.name = QT_TRANSLATE_NOOP("QObject", "Numerator"),
                      .value = interval.ratio.numerator,
                      .maximum = MAX_NUMERATOR,
                      .comparison = QT_TRANSLATE_NOOP(
                          "QObject", " greater than maximum ")},
        IntervalLimit{.name = QT_TRANSLATE_NOOP("QObject", "Denominator"),
                      .value = interval.ratio.denominator,
                      .maximum = MAX_DENOMINATOR,
                      .comparison = QT_TRANSLATE_NOOP(
                          "QObject", " greater than maximum ")},
        IntervalLimit{.name = QT_TRANSLATE_NOOP("QObject", "Octave"),
                      .value = interval.octave,
                      .maximum = MAX_OCTAVE,
                      .comparison = QT_TRANSLATE_NOOP(
                          "QObject", " (absolutely) greater than maximum ")}}) {
    if (std::abs(limit.value) > limit.maximum) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr(limit.name) << " " << limit.value
             << QObject::tr(limit.comparison) << limit.maximum;
      QMessageBox::warning(&parent_widget,
                           QObject::tr("%1 error").arg(QObject::tr(limit.name)),
                           message);
      return false;
    }
  }
  return true;
}
