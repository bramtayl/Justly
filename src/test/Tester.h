#pragma once

#include <qobject.h>         // for QObject
#include <qtemporaryfile.h>  // for QTemporaryFile
#include <qtmetamacros.h>    // for Q_OBJECT, slots

#include <memory>  // for make_unique, unique_ptr

#include "main/Editor.h"          // for Editor
#include "notechord/NoteChord.h"  // for NoteChordField

class QModelIndex;

class Tester : public QObject {
  Q_OBJECT

 private:
  QTemporaryFile main_file;

  std::unique_ptr<Editor> editor_pointer = std::make_unique<Editor>();

  [[nodiscard]] auto get_index(int = -1, int = -1,
                               NoteChordField = symbol_column) const
      -> QModelIndex;
  void select_index(QModelIndex) const;
  void select_indices(QModelIndex, QModelIndex) const;
  void clear_selection() const;
  static void close_one_message();

 private slots:
  void initTestCase();
  void test_column_headers() const;
  void test_insert_delete();
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
  void test_io() const;
  void test_controls_template() const;
  static void test_controls_template_data();
  void test_delegate_template();
  void test_delegate_template_data();
  void test_select();
};
