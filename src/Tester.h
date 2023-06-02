#pragma once

#include <qnamespace.h>      // for ItemDataRole
#include <qobject.h>         // for QObject
#include <qtemporaryfile.h>  // for QTemporaryFile
#include <qtmetamacros.h>    // for Q_OBJECT, slots
#include <qvariant.h>        // for QVariant

#include "Editor.h"  // for Editor
class QModelIndex;   // lines 10-10

class Tester : public QObject {
  Q_OBJECT
 public:
  Editor editor;
  QTemporaryFile test_file;

  void assert_is_gray(int row, int column, QModelIndex &parent_index);
  void assert_is_not_gray(int row, int column, QModelIndex &parent_index);

  auto get_data(int row, int column, QModelIndex &parent_index,
                Qt::ItemDataRole role) -> QVariant;
  auto set_data(int row, int column, QModelIndex &parent_index,
                const QVariant &new_value) -> bool;
  void run_actions(QModelIndex &parent_index);
  auto get_column_heading(int column) const -> QVariant;

 private slots:
  void test_column_headers();
  void test_song();
  void test_chord();
  void test_note();
  void test_actions();
  void initTestCase();
  void test_colors();
  void test_data();
  void test_set_data();
};
