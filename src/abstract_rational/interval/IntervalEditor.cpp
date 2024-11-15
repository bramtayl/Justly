#include "abstract_rational/interval/IntervalEditor.hpp"

#include <QBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QString>

#include "abstract_rational/AbstractRationalEditor.hpp"
#include "abstract_rational/interval/Interval.hpp"
#include "other/other.hpp"

class QWidget;

IntervalEditor::IntervalEditor(QWidget *parent_pointer)
    : AbstractRationalEditor(parent_pointer),
      octave_box(*(new QSpinBox)) {
  octave_box.setMinimum(-MAX_OCTAVE);
  octave_box.setMaximum(MAX_OCTAVE);

  row_layout.addWidget(
      new QLabel("o")); // NOLINT(cppcoreguidelines-owning-memory)
  row_layout.addWidget(&octave_box);

  prevent_compression(*this);
}

auto IntervalEditor::value() const -> Interval {
  return {numerator_box.value(), denominator_box.value(), octave_box.value()};
}

void IntervalEditor::setValue(const Interval &new_value) const {
  AbstractRationalEditor::setValue(new_value);
  octave_box.setValue(new_value.octave);
}
