#pragma once

#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>

#include "cell_types/PercussionInstrument.hpp"
#include "cell_editors/ProgramEditor.hpp"

static const auto MAX_MIDI_NUMBER = 127;

struct PercussionInstrumentEditor : public QFrame {
  Q_OBJECT
  Q_PROPERTY(PercussionInstrument value READ value WRITE setValue USER true)

  ProgramEditor &percussion_set_editor = *(new ProgramEditor(this, false));
  QLabel &number_text = *(new QLabel("#"));
  QSpinBox &midi_number_box = *(new QSpinBox);
  QBoxLayout &row_layout = *(new QHBoxLayout(this));

public:
  explicit PercussionInstrumentEditor(QWidget *const parent_pointer)
      : QFrame(parent_pointer) {
    setFrameStyle(QFrame::StyledPanel);
    setAutoFillBackground(true);

    midi_number_box.setMinimum(0);
    midi_number_box.setMaximum(MAX_MIDI_NUMBER);

    row_layout.addWidget(&percussion_set_editor);
    row_layout.addWidget(&number_text);
    row_layout.addWidget(&midi_number_box);
    row_layout.setContentsMargins(0, 0, 0, 0);
  }
  [[nodiscard]] auto value() const -> PercussionInstrument {
    return PercussionInstrument(percussion_set_editor.value(),
                                static_cast<short>(midi_number_box.value()));
  }
  void setValue(const PercussionInstrument &new_value) const {
    percussion_set_editor.setValue(new_value.percussion_set_pointer);
    midi_number_box.setValue(new_value.midi_number);
  }
};