#include "rational/RationalEditor.hpp"

#include <QBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QString>

#include "other/other.hpp"
#include "rational/Rational.hpp"

class QWidget;

RationalEditor::RationalEditor(QWidget *parent_pointer)
    : AbstractRationalEditor(parent_pointer) {
  auto &row = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QHBoxLayout(this));
  row.addWidget(&numerator_box);
  row.addWidget(
      new QLabel("/", this)); // NOLINT(cppcoreguidelines-owning-memory)
  row.addWidget(&denominator_box);

  prevent_compression(*this);
}

auto RationalEditor::value() const -> Rational {
  return {numerator_box.value(), denominator_box.value()};
}
