#pragma once

#include <QtCore/QStringListModel>
#include <QtWidgets/QComboBox>
#include <algorithm>

struct StringPicker : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(const QString &value READ currentText WRITE setValue USER true)

public:
  const QList<QString> strings;
  QStringListModel voice_model;
  explicit StringPicker(QWidget *const parent_pointer,
                        QList<QString> input_voice_names)
      : QComboBox(parent_pointer), strings(std::move(input_voice_names)) {
    voice_model.setStringList(strings);
    setModel(&voice_model);
    // force scrollbar for combo box
    setStyleSheet("combobox-popup: 0;");
  }

  [[nodiscard]] auto value() const -> const auto & {
    return strings.at(currentIndex());
  }

  void setValue(const QString &new_value) {
    const auto iterator = std::ranges::find(strings, new_value);
    Q_ASSERT(iterator != strings.cend());
    setCurrentIndex(
        static_cast<int>(std::distance(strings.cbegin(), iterator)));
  }
};
