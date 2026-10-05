#pragma once

#include <QtWidgets/QComboBox>

struct StringPicker : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(const QString& value READ currentText WRITE setValue USER true)

 public:
  explicit StringPicker(QWidget* parent_pointer,
                        const QList<QString>& input_voice_names);

  void setValue(const QString& new_value);
};
