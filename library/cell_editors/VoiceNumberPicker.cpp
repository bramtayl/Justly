#include "cell_editors/VoiceNumberPicker.hpp"

VoiceNumberPicker::VoiceNumberPicker(QWidget* const parent_pointer,
                                     const QList<QString>& voice_names)
    : QComboBox(parent_pointer) {
  addItems(voice_names);
  // force scrollbar for combo box
  setStyleSheet("combobox-popup: 0;");
}
