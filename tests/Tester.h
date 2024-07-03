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

  void test_playback_volume_control();
  void test_starting_instrument_control() const;
  void test_starting_key_control() const;
  void test_starting_volume_control() const;
  void test_starting_tempo_control() const;

  void test_tree() const;

  void test_copy_paste() const;
  void test_insert_delete() const;

  void test_column_headers_template() const;
  static void test_column_headers_template_data();

  void test_select_template() const;
  void test_select_template_data() const;

  void test_flags_template() const;
  void test_flags_template_data() const;

  void test_colors_template() const;
  void test_colors_template_data() const;

  void test_get_value_template() const;
  void test_get_value_template_data() const;

  void test_delegate_template() const;
  void test_delegate_template_data() const;

  void test_set_value() const;
  void test_set_value_template() const;
  void test_set_value_template_data() const;

  void test_io();

  void test_play();
  void test_play_template() const;
  void test_play_template_data() const;
};
