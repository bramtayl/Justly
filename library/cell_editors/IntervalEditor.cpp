#include "cell_editors/IntervalEditor.hpp"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>

#include "cell_editors/RationalEditor.hpp"

IntervalEditor::IntervalEditor(QWidget* const parent_pointer)
    : QFrame(parent_pointer),
      rational_editor(*(new RationalEditor(parent_pointer))),
      octave_box(*(new QSpinBox)) {
  setFrameStyle(QFrame::StyledPanel);
  setAutoFillBackground(true);

  rational_editor.setFrameShape(QFrame::NoFrame);

  octave_box.setMinimum(-MAX_OCTAVE);
  octave_box.setMaximum(MAX_OCTAVE);

  auto& row_layout =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QHBoxLayout(this));
  row_layout.addWidget(&rational_editor);
  row_layout.addWidget(
      new QLabel("o"));  // NOLINT(cppcoreguidelines-owning-memory)
  row_layout.addWidget(&octave_box);
  row_layout.setContentsMargins(1, 0, 1, 0);
}

auto IntervalEditor::value() const -> Interval {
  return Interval(rational_editor.value(), octave_box.value());
}

void IntervalEditor::setValue(const Interval& new_value) const {
  rational_editor.setValue(new_value.ratio);
  octave_box.setValue(new_value.octave);
}
