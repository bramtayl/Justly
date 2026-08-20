#include "cell_editors/MidiNumberEditor.hpp"

MidiNumberEditor::MidiNumberEditor(QWidget* const parent_pointer)
    : QSpinBox(parent_pointer) {
  static const auto MAX_MIDI_NUMBER = 127;
  setMinimum(0);
  setMaximum(MAX_MIDI_NUMBER);
}

auto MidiNumberEditor::get_short_value() const -> short {
  return static_cast<short>(value());
}

void MidiNumberEditor::set_short_value(const short new_value) {
  setValue(static_cast<short>(new_value));
}
