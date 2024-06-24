#pragma once

#include <qobject.h>         // for QObject
#include <qtemporaryfile.h>  // for QTemporaryFile
#include <qtmetamacros.h>    // for Q_OBJECT, slots

#include "justly/SongEditor.hpp"  // for SongEditor

class Tester : public QObject {
  Q_OBJECT

  QTemporaryFile main_file;
  SongEditor song_editor;

 private slots:
  void initTestCase();
  static void test_interval();
  static void test_rational();

  void test_starting_instrument_control();
  void test_starting_key_control();
  void test_starting_volume_control();
  void test_starting_tempo_control();

  void test_tree();

  void test_copy_paste();
  void test_insert_delete();

  void test_column_headers() const;
  void test_column_headers_template() const;
  static void test_column_headers_template_data();

  void test_select();

  void test_flags() const;

  void test_colors_template();
  void test_colors_template_data();

  void test_get_value_template();
  void test_get_value_template_data();

  void test_delegate_template();
  void test_delegate_template_data();

  void test_set_value();
  void test_set_value_template();
  void test_set_value_template_data();

  void test_io();

  void test_play();
  void test_play_template();
  void test_play_template_data() const;
};
