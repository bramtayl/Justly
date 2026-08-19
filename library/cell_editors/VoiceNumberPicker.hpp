#pragma once

#include <QtCore/qtmetamacros.h>

#include <QtWidgets/QComboBox>

class QString;
class QWidget;
template <typename T>
class QList;

struct VoiceNumberPicker : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(int voice_number READ currentIndex WRITE setCurrentIndex USER true)

 public:
  explicit VoiceNumberPicker(QWidget* parent_pointer,
                             const QList<QString>& voice_names);

  ~VoiceNumberPicker() override = default;
};
