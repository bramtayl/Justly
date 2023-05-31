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
  QTemporaryFile test_file;

  void assert_is_gray(int row, int column, QModelIndex &parent_index);
  void assert_is_not_gray(int row, int column, QModelIndex &parent_index);

  auto get_data(int row, int column, QModelIndex &parent_index, Qt::ItemDataRole role) -> QVariant;
  auto set_data(int row, int column, QModelIndex &parent_index,
                const QVariant &new_value) -> void;
  void set_unset_field(int row, int column, QModelIndex &parent_index,
                const QVariant& expected_value, const QVariant &new_value);
  void set_unset_picky_field(int row, int column, QModelIndex &parent_index,
                      const QVariant &expected_value,
                      const QVariant &invalid_value, const QVariant &valid_value);
  void run_actions(QModelIndex& parent_index);
  auto get_column_heading(int column) -> QVariant;
  
 private slots:
  void test_column_headers();
  void test_song();
  void test_chord();
  void test_note();
  void test_sliders();
  void test_actions();
  void initTestCase();
  void test_colors();
};
