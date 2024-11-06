#include "interval/IntervalEditor.hpp"

#include <QBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QString>

#include "interval/Interval.hpp"
#include "other/other.hpp"
#include "rational/AbstractRationalEditor.hpp"

class QWidget;

IntervalEditor::IntervalEditor(QWidget *parent_pointer)
    : AbstractRationalEditor(parent_pointer),
      octave_box(*(new QSpinBox(this))) {
  octave_box.setMinimum(-MAX_OCTAVE);
  octave_box.setMaximum(MAX_OCTAVE);

  auto &row = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QHBoxLayout(this));
  row.addWidget(&numerator_box);
  row.addWidget(
      new QLabel("/", this)); // NOLINT(cppcoreguidelines-owning-memory)
  row.addWidget(&denominator_box);
  row.addWidget(
      new QLabel("o", this)); // NOLINT(cppcoreguidelines-owning-memory)
  row.addWidget(&octave_box);

  prevent_compression(*this);
}

auto IntervalEditor::value() const -> Interval {
  return {numerator_box.value(), denominator_box.value(), octave_box.value()};
}

void IntervalEditor::setValue(const Interval &new_value) const {
  AbstractRationalEditor::setValue(new_value);
  octave_box.setValue(new_value.octave);
}
