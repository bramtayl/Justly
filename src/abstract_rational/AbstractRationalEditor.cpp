#include "abstract_rational/AbstractRationalEditor.hpp"

#include <QBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QSpinBox>
#include <QString>

#include "abstract_rational/AbstractRational.hpp"

class QWidget;

AbstractRationalEditor::AbstractRationalEditor(QWidget *parent_pointer)
    : QFrame(parent_pointer), numerator_box(*(new QSpinBox(this))),
      denominator_box(*(new QSpinBox(this))), 
      row_layout(*(new QHBoxLayout(this))) {
  setFrameStyle(QFrame::StyledPanel);
  setAutoFillBackground(true);

  numerator_box.setMinimum(1);
  numerator_box.setMaximum(MAX_RATIONAL_NUMERATOR);

  denominator_box.setMinimum(1);
  denominator_box.setMaximum(MAX_RATIONAL_DENOMINATOR);

  row_layout.addWidget(&numerator_box);
  row_layout.addWidget(
      new QLabel("/", this)); // NOLINT(cppcoreguidelines-owning-memory)
  row_layout.addWidget(&denominator_box);
}

void AbstractRationalEditor::setValue(const AbstractRational &new_value) const {
  numerator_box.setValue(new_value.numerator);
  denominator_box.setValue(new_value.denominator);
}

