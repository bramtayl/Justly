#include "interval/IntervalEditor.hpp"

#include <QBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QString>

#include "interval/Interval.hpp"
#include "other/other.hpp"

class QWidget;

IntervalEditor::IntervalEditor(QWidget *parent_pointer)
    : AbstractRationalEditor(parent_pointer),
      octave_box(*(new QSpinBox(this))) {
  octave_box.setMinimum(-MAX_OCTAVE);
  octave_box.setMaximum(MAX_OCTAVE);

  auto &row_pointer = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QHBoxLayout(this));
  row_pointer.addWidget(&numerator_box);
  row_pointer.addWidget(
      new QLabel("/", this)); // NOLINT(cppcoreguidelines-owning-memory)
  row_pointer.addWidget(&denominator_box);
  row_pointer.addWidget(
      new QLabel("o", this)); // NOLINT(cppcoreguidelines-owning-memory)
  row_pointer.addWidget(&octave_box);

  set_minimum_size(*this);
}

auto IntervalEditor::value() const -> Interval {
  return {numerator_box.value(), denominator_box.value(), octave_box.value()};
}

void IntervalEditor::setValue(const Interval &new_value) const {
  numerator_box.setValue(new_value.numerator);
  denominator_box.setValue(new_value.denominator);
  octave_box.setValue(new_value.octave);
}
