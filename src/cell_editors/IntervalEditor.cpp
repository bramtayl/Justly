#include "cell_editors/IntervalEditor.hpp"

#include <QBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QSpinBox>
#include <QtGlobal>
#include <memory>

#include "justly/Interval.hpp"
#include "other/private.hpp"

auto get_interval_size() -> QSize {
  static auto interval_size = IntervalEditor().sizeHint();
  return interval_size;
};

IntervalEditor::IntervalEditor(QWidget *parent_pointer_input)
    : QFrame(parent_pointer_input) {
  setFrameStyle(QFrame::StyledPanel);
  setAutoFillBackground(true);

  Q_ASSERT(numerator_box_pointer != nullptr);
  numerator_box_pointer->setMinimum(1);
  numerator_box_pointer->setMaximum(MAX_NUMERATOR);

  Q_ASSERT(denominator_box_pointer != nullptr);
  denominator_box_pointer->setMinimum(1);
  denominator_box_pointer->setMaximum(MAX_DENOMINATOR);

  Q_ASSERT(octave_box_pointer != nullptr);
  octave_box_pointer->setMinimum(MIN_OCTAVE);
  octave_box_pointer->setMaximum(MAX_OCTAVE);

  auto *row_pointer = std::make_unique<QHBoxLayout>(this).release();
  row_pointer->addWidget(numerator_box_pointer);
  row_pointer->addWidget(std::make_unique<QLabel>("/", this).release());
  row_pointer->addWidget(denominator_box_pointer);
  row_pointer->addWidget(std::make_unique<QLabel>("o", this).release());
  row_pointer->addWidget(octave_box_pointer);
  row_pointer->setContentsMargins(SMALL_SPACING, SMALL_SPACING, SMALL_SPACING,
                                  SMALL_SPACING);

  setLayout(row_pointer);

  setMinimumSize(sizeHint());
}

auto IntervalEditor::value() const -> Interval {
  Q_ASSERT(numerator_box_pointer != nullptr);
  Q_ASSERT(denominator_box_pointer != nullptr);
  Q_ASSERT(octave_box_pointer != nullptr);
  return Interval(numerator_box_pointer->value(),
                  denominator_box_pointer->value(),
                  octave_box_pointer->value());
}

void IntervalEditor::setValue(Interval new_value) const {
  Q_ASSERT(numerator_box_pointer != nullptr);
  Q_ASSERT(denominator_box_pointer != nullptr);
  Q_ASSERT(octave_box_pointer != nullptr);

  numerator_box_pointer->setValue(new_value.numerator);
  denominator_box_pointer->setValue(new_value.denominator);
  octave_box_pointer->setValue(new_value.octave);
}
