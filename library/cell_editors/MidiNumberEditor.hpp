#pragma once

#include <QtWidgets/QSpinBox>

struct MidiNumberEditor : QSpinBox {
  Q_OBJECT
  Q_PROPERTY(
      short midi_number READ get_short_value WRITE set_short_value USER true)

 public:
  explicit MidiNumberEditor(QWidget* parent_pointer);

  ~MidiNumberEditor() override = default;

  [[nodiscard]] auto get_short_value() const -> short;

  void set_short_value(short new_value);
};
