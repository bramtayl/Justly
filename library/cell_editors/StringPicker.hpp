#pragma once

#include <QtCore/QStringListModel>
#include <QtWidgets/QComboBox>

struct StringPicker : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(const QString& value READ currentText WRITE setValue USER true)

 public:
  const QList<QString> strings;
  QStringListModel voice_model;
  explicit StringPicker(QWidget* parent_pointer,
                        QList<QString> input_voice_names);

  void setValue(const QString& new_value);
};
