#include "rational/AbstractRationalEditor.hpp"

#include <QFrame>
#include <QSpinBox>

#include "rational/AbstractRational.hpp"

class QWidget;

AbstractRationalEditor::AbstractRationalEditor(QWidget *parent_pointer)
    : QFrame(parent_pointer), numerator_box(*(new QSpinBox(this))),
      denominator_box(*(new QSpinBox(this))) {
  setFrameStyle(QFrame::StyledPanel);
  setAutoFillBackground(true);

  numerator_box.setMinimum(1);
  numerator_box.setMaximum(MAX_RATIONAL_NUMERATOR);

  denominator_box.setMinimum(1);
  denominator_box.setMaximum(MAX_RATIONAL_DENOMINATOR);
}

void AbstractRationalEditor::setValue(const AbstractRational &new_value) const {
  numerator_box.setValue(new_value.numerator);
  denominator_box.setValue(new_value.denominator);
}

