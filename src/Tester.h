#pragma once

#include <qdir.h>          // for QDir
#include <qobject.h>       // for QObject
#include <qstring.h>       // for QString
#include <qtmetamacros.h>  // for Q_OBJECT, slots
#include <qvariant.h>      // for QVariant

#include "Editor.h"  // for Editor
class QModelIndex;   // lines 9-9

class Tester : public QObject {
  Q_OBJECT
 public:
  Editor editor;

  void test_maybe_set(int row, int column, QModelIndex &parent_index,
                      const QVariant new_value);
  auto get_data(int row, int column, QModelIndex &parent_index) -> QVariant;
  auto set_data(int row, int column, QModelIndex &parent_index,
                const QVariant &new_value) -> void;
  void test_set(int row, int column, QModelIndex &parent_index,
                const QVariant& expected_value, const QVariant &new_value);
  void test_maybe_set(int row, int column, QModelIndex &parent_index,
                      const QVariant &expected_value,
                      const QVariant &invalid_value, const QVariant &valid_value);
 private slots:
  void test_everything();
};
