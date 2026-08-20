#pragma once

#include <QtWidgets/QComboBox>

struct VoiceNumberPicker : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(int voice_number READ currentIndex WRITE setCurrentIndex USER true)

 public:
  explicit VoiceNumberPicker(QWidget* parent_pointer,
                             const QList<QString>& voice_names);

  ~VoiceNumberPicker() override = default;
};
