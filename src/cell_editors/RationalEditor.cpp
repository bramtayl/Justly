#include "cell_editors/RationalEditor.hpp"

#include <QBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QSpinBox>
#include <QtGlobal>
#include <memory>

#include "justly/Rational.hpp"
#include "other/private.hpp"

auto get_rational_size() -> QSize {
  static auto rational_size = RationalEditor().sizeHint();
  return rational_size;
};

RationalEditor::RationalEditor(QWidget *parent_pointer_input)
    : QFrame(parent_pointer_input) {
  setFrameStyle(QFrame::StyledPanel);
  setAutoFillBackground(true);

  Q_ASSERT(numerator_box_pointer != nullptr);
  numerator_box_pointer->setMinimum(1);
  numerator_box_pointer->setMaximum(MAX_NUMERATOR);

  Q_ASSERT(denominator_box_pointer != nullptr);
  denominator_box_pointer->setMinimum(1);
  denominator_box_pointer->setMaximum(MAX_DENOMINATOR);

  auto *row_pointer = std::make_unique<QHBoxLayout>(this).release();
  row_pointer->addWidget(numerator_box_pointer);
  row_pointer->addWidget(std::make_unique<QLabel>("/", this).release());
  row_pointer->addWidget(denominator_box_pointer);
  row_pointer->setContentsMargins(SMALL_SPACING, SMALL_SPACING, SMALL_SPACING,
                                  SMALL_SPACING);
  setLayout(row_pointer);
}

auto RationalEditor::value() const -> Rational {
  Q_ASSERT(numerator_box_pointer != nullptr);
  Q_ASSERT(denominator_box_pointer != nullptr);
  return Rational(numerator_box_pointer->value(),
                  denominator_box_pointer->value());
}

void RationalEditor::setValue(Rational new_value) const {
  Q_ASSERT(numerator_box_pointer != nullptr);
  numerator_box_pointer->setValue(new_value.numerator);

  Q_ASSERT(denominator_box_pointer != nullptr);
  denominator_box_pointer->setValue(new_value.denominator);
}
