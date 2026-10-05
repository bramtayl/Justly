#include "cell_editors/StringPicker.hpp"

StringPicker::StringPicker(QWidget* const parent_pointer,
                           const QList<QString>& input_voice_names)
    : QComboBox(parent_pointer) {
  addItems(input_voice_names);
  // force scrollbar for combo box
  setStyleSheet("combobox-popup: 0;");
}

void StringPicker::setValue(const QString& new_value) {
  const auto index = findText(new_value);
  Q_ASSERT(index != -1);
  setCurrentIndex(index);
}
