#pragma once

#include <QItemSelectionModel>
#include <QObject>
#include <QString>

#include "justly/SongEditor.hpp"

class QAbstractItemModel;
class QAction;
class QDoubleSpinBox;
class QModelIndex;
class QUndoStack;

class Tester : public QObject {
  Q_OBJECT

private:
  SongEditor song_editor;

  QItemSelectionModel *const selector_pointer =
      song_editor.get_selection_model();
  QAbstractItemModel *const chords_model_pointer =
      song_editor.get_chords_model();

  bool waiting_for_message = false;
  void close_message_later(const QString &expected_text);
  void clear_selection() const;

public:
private slots:
  void initTestCase();

  static void test_to_string_template();
  static void test_to_string_template_data();

  void test_row_count_template();
  void test_row_count_template_data();

  void test_parent_template();
  void test_parent_template_data();

  void test_column_count() const;

  void test_playback_volume_control();
  void test_starting_instrument_control() const;
  void test_starting_key_control() const;
  void test_starting_volume_control() const;
  void test_starting_tempo_control() const;

  void test_column_headers_template() const;
  static void test_column_headers_template_data();

  void test_select_template() const;
  void test_select_template_data() const;

  void test_flags_template() const;
  void test_flags_template_data() const;

  void test_get_value_template() const;
  void test_get_value_template_data() const;

  void get_different_values_template() const;
  void get_different_values_template_data() const;

  void test_delegate_template() const;
  void test_delegate_template_data() const;

  void test_set_value() const;
  void test_set_value_template() const;
  void test_set_value_template_data() const;

  void test_delete_cell_template();
  void test_delete_cell_template_data();

  void test_delete_cells_template();
  void test_delete_cells_template_data();

  void test_delete_cells_3_template();
  void test_delete_cells_3_template_data();

  void test_paste_cell_template();
  void test_paste_cell_template_data();

  void test_paste_cells_template();
  void test_paste_cells_template_data();

  void test_paste_truncate_template();
  void test_paste_truncate_template_data();

  void test_paste_recycle_template();
  void test_paste_recycle_template_data();

  void test_insert_delete() const;
  void test_insert_rows_template();
  void test_insert_rows_template_data();

  void test_delete_rows_template();
  void test_delete_rows_template_data();

  void test_paste_rows_template();
  void test_paste_rows_template_data();

  void test_bad_paste_template();
  void test_bad_paste_template_data();

  void test_paste_rows();

  void test_play();
  void test_play_template() const;
  void test_play_template_data() const;

  void test_io();
};
