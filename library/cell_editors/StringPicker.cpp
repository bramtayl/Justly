#include "cell_editors/StringPicker.hpp"

StringPicker::StringPicker(QWidget *const parent_pointer,
                           QList<QString> input_voice_names)
    : QComboBox(parent_pointer), strings(std::move(input_voice_names)) {
  voice_model.setStringList(strings);
  setModel(&voice_model);
  // force scrollbar for combo box
  setStyleSheet("combobox-popup: 0;");
}

void StringPicker::setValue(const QString &new_value) {
  const auto iterator = std::ranges::find(strings, new_value);
  Q_ASSERT(iterator != strings.cend());
  setCurrentIndex(
      static_cast<int>(std::distance(strings.cbegin(), iterator)));
}
