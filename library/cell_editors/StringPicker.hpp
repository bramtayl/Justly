#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringListModel>
#include <QtCore/QtAssert>
#include <QtCore/qtmetamacros.h>
#include <QtWidgets/QComboBox>
#include <algorithm>
#include <iterator>
#include <utility>

class QWidget;

struct StringPicker : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(const QString &value READ currentText WRITE setValue USER true)

public:
  const QList<QString> strings;
  QStringListModel voice_model;
  explicit StringPicker(QWidget *const parent_pointer,
                        QList<QString> input_voice_names);

  void setValue(const QString &new_value);
};
