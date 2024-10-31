#include "interval/IntervalEditor.hpp"

#include <QBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QSpinBox>
#include <QString>

#include "interval/Interval.hpp"

class QWidget;

IntervalEditor::IntervalEditor(QWidget *parent_pointer)
    : QFrame(parent_pointer), numerator_box(*(new QSpinBox(this))),
      denominator_box(*(new QSpinBox(this))),
      octave_box(*(new QSpinBox(this))) {
  setFrameStyle(QFrame::StyledPanel);
  setAutoFillBackground(true);

  numerator_box.setMinimum(1);
  numerator_box.setMaximum(MAX_INTERVAL_NUMERATOR);

  denominator_box.setMinimum(1);
  denominator_box.setMaximum(MAX_INTERVAL_DENOMINATOR);

  octave_box.setMinimum(MIN_OCTAVE);
  octave_box.setMaximum(MAX_OCTAVE);

  auto& row_pointer = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QHBoxLayout(this));
  row_pointer.addWidget(&numerator_box);
  row_pointer.addWidget(
      new QLabel("/", this)); // NOLINT(cppcoreguidelines-owning-memory)
  row_pointer.addWidget(&denominator_box);
  row_pointer.addWidget(
      new QLabel("o", this)); // NOLINT(cppcoreguidelines-owning-memory)
  row_pointer.addWidget(&octave_box);

  setMinimumSize(sizeHint());
}

auto IntervalEditor::value() const -> Interval {
  return Interval({numerator_box.value(),
                   denominator_box.value(),
                   octave_box.value()});
}

void IntervalEditor::setValue(Interval new_value) const {
  numerator_box.setValue(new_value.numerator);
  denominator_box.setValue(new_value.denominator);
  octave_box.setValue(new_value.octave);
}
