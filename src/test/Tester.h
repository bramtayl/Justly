#pragma once

#include <qobject.h>             // for QObject
#include <qstring.h>             // for QString
#include <qtemporaryfile.h>      // for QTemporaryFile
#include <qtmetamacros.h>        // for Q_OBJECT, slots
#include <qvariant.h>            // for QVariant

#include <memory>  // for make_unique, unique_ptr

#include "main/Editor.h"  // for Editor

class QModelIndex;

class Tester : public QObject {
  Q_OBJECT

 private:
  QTemporaryFile main_file;

  std::unique_ptr<Editor> editor_pointer = std::make_unique<Editor>();

  [[nodiscard]] auto get_column_heading(int column) const -> QVariant;
  void select_index(QModelIndex index) const;
  void select_indices(QModelIndex first_index, QModelIndex last_index) const;
  void clear_selection() const;
  void save_to(const QString &filename) const;
 private slots:
  static void close_one_message();
  void initTestCase();
  void test_column_headers() const;
  void test_insert_delete();
  void test_colors();
  void test_copy_paste();
  void test_get_value();
  void test_set_value();
  void test_colors_template();
  void test_colors_template_data();
  void test_set_value_template();
  void test_set_value_template_data();
  void test_play() const;
  void test_play_template() const;
  void test_play_template_data() const;
  void test_column_headers_template() const;
  static void test_column_headers_template_data();
  void test_flags() const;
  void test_tree();
  void test_json();
  void test_io() const;
  void test_controls_template() const;
  static void test_controls_template_data();
  void test_delegate_template();
  void test_delegate_template_data();
  void test_select();
};
