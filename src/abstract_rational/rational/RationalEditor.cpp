#include "abstract_rational/rational/RationalEditor.hpp"

#include <QSpinBox>

#include "abstract_rational/rational/Rational.hpp"
#include "other/other.hpp"

class QWidget;

RationalEditor::RationalEditor(QWidget *parent_pointer)
    : AbstractRationalEditor(parent_pointer) {
  prevent_compression(*this);
}

auto RationalEditor::value() const -> Rational {
  return {numerator_box.value(), denominator_box.value()};
}
