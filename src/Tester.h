#pragma once

#include <qdir.h>          // for QDir
#include <qobject.h>       // for QObject
#include <qstring.h>       // for QString
#include <qtmetamacros.h>  // for Q_OBJECT, slots
#include "Editor.h"        // for Editor
class QModelIndex;  // lines 8-8

class Tester : public QObject {
  Q_OBJECT
 public:
  Editor editor;
  const QDir examples_folder;

  explicit Tester(const QString &examples_folder_input);

  void test_positive_int_field(int row, int column, QModelIndex &parent_index);
  void test_int_field(int row, int column, QModelIndex &parent_index);
  void test_positive_double_field(int row, int column,
                                  QModelIndex &parent_index);
  void test_string_field(int row, int column, QModelIndex &parent_index);
 private slots:
  void test_everything();
};
