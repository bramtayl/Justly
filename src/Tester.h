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

  auto get_data(int row, int column, QModelIndex &parent_index,
                Qt::ItemDataRole role) -> QVariant;
  auto set_data(int row, int column, QModelIndex &parent_index,
                const QVariant &new_value) -> bool;
  void run_actions(QModelIndex &parent_index);
  auto get_column_heading(int column) const -> QVariant;

 private slots:
  void initTestCase();
  void test_column_headers();
  void test_insert_delete();
  void test_colors();
  void test_get_value();
  void test_set_value();
  void test_play();
  void test_flags();
  void test_tree();
  void test_save();
  void test_misc();
  void test_orchestra();
  void test_sliders();
  void dismiss_messages();
  void dismiss_save_orchestra_text();
  void timer_save_orchestra_text();
};
