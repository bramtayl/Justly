#pragma once

#include <QtWidgets/QSpinBox>

static const auto MAX_MIDI_NUMBER = 127;

struct MidiNumberEditor : QSpinBox {
  Q_OBJECT
  Q_PROPERTY(
      short midi_number READ get_short_value WRITE set_short_value USER true)

public:
  explicit MidiNumberEditor(QWidget *const parent_pointer)
      : QSpinBox(parent_pointer) {
    setMinimum(0);
    setMaximum(MAX_MIDI_NUMBER);
  }

   ~MidiNumberEditor() override = default;

  [[nodiscard]] auto get_short_value() const {
    return static_cast<short>(value());
  }

  void set_short_value(const short new_value) {
    setValue(static_cast<short>(new_value));
  }
};