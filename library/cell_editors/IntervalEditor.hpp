#pragma once

#include "cell_editors/RationalEditor.hpp"
#include "cell_types/Interval.hpp"

static const auto MAX_OCTAVE = 9;

struct IntervalEditor : QFrame {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ value WRITE setValue USER true)

public:
  RationalEditor &rational_editor;

  QWidget &o_text = *(new QLabel("o"));
  QSpinBox &octave_box = *(new QSpinBox);
  QBoxLayout &row_layout = *(new QHBoxLayout(this));

  explicit IntervalEditor(QWidget *const parent_pointer)
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

  [[nodiscard]] auto value() const {
    return Interval(rational_editor.value(), octave_box.value());
  }

  void setValue(const Interval &new_value) const {
    rational_editor.setValue(new_value.ratio);
    octave_box.setValue(new_value.octave);
  }
};