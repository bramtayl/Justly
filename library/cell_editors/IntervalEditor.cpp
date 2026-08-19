#include "cell_editors/IntervalEditor.hpp"

IntervalEditor::IntervalEditor(QWidget *const parent_pointer)
    : QFrame(parent_pointer),
      rational_editor(*(new RationalEditor(parent_pointer))) {

  setFrameStyle(QFrame::StyledPanel);
  setAutoFillBackground(true);

  rational_editor.setFrameShape(QFrame::NoFrame);

  octave_box.setMinimum(-MAX_OCTAVE);
  octave_box.setMaximum(MAX_OCTAVE);

  row_layout.addWidget(&rational_editor);
  row_layout.addWidget(&o_text);
  row_layout.addWidget(&octave_box);
  row_layout.setContentsMargins(1, 0, 1, 0);
}

auto IntervalEditor::value() const -> Interval {
  return Interval(rational_editor.value(), octave_box.value());
}

void IntervalEditor::setValue(const Interval &new_value) const {
  rational_editor.setValue(new_value.ratio);
  octave_box.setValue(new_value.octave);
}
