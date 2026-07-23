#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QComboBox>

class QString;
class QWidget;
template <typename T> class QList;

struct VoiceNumberPicker : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(
      int voice_number READ currentIndex WRITE setCurrentIndex USER true)

public:
  explicit VoiceNumberPicker(QWidget *const parent_pointer,
                             const QList<QString> &voice_names)
      : QComboBox(parent_pointer) {
    addItems(voice_names);
    // force scrollbar for combo box
    setStyleSheet("combobox-popup: 0;");
  }

  ~VoiceNumberPicker() override = default;
};
