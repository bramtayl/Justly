#pragma once

#include <QtCore/QStringListModel>
#include <QtWidgets/QComboBox>

struct VoiceEditor : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(QString value READ currentText WRITE setValue USER true)

public:
  const QList<QString> voice_names;
  QStringListModel voice_model;
  explicit VoiceEditor(QWidget *const parent_pointer,
                       const QList<QString> &input_voice_names)
      : QComboBox(parent_pointer), voice_names(input_voice_names) {
    voice_model.setStringList(voice_names);
    setModel(&voice_model);
    // force scrollbar for combo box
    setStyleSheet("combobox-popup: 0;");
  }

  [[nodiscard]] auto value() const -> QString {
    return voice_names.at(currentIndex());
  }

  void setValue(const QString &new_value) {
    const auto iterator =
        std::find(voice_names.cbegin(), voice_names.cend(), new_value);
    Q_ASSERT(iterator != voice_names.cend());
    setCurrentIndex(
        static_cast<int>(std::distance(voice_names.cbegin(), iterator)));
  }
};
