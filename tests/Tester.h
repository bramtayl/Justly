#pragma once

#include <QObject>
#include <QString>

class QAbstractItemModel;
class QItemSelectionModel;
class QTreeView;
class SongEditor;

class Tester : public QObject {
  Q_OBJECT

private:
  SongEditor* const song_editor_pointer;

  QTreeView *const chords_view_pointer;
  QItemSelectionModel *const selector_pointer;
  QAbstractItemModel *const chords_model_pointer;

  bool waiting_for_message = false;
  void close_message_later(const QString &expected_text);
  void clear_selection() const;
  void open_text(const QString& json_song);

public:
  Tester();
  ~Tester() override;
private slots:
  void initTestCase();

  void test_to_string_template();
  void test_to_string_template_data();

  void test_row_count_template();
  void test_row_count_template_data();

  void test_parent_template();
  void test_parent_template_data();

  void test_column_count() const;

  void test_gain_control();
  void test_starting_instrument_control() const;
  void test_starting_key_control() const;
  void test_starting_velocity_control() const;
  void test_starting_tempo_control() const;

  void test_column_headers_template() const;
  static void test_column_headers_template_data();

  void test_select_template() const;
  void test_select_template_data() const;

  void test_flags_template() const;
  void test_flags_template_data() const;

  void test_status_template();
  static void test_status_template_data();

  void test_get_value_template() const;
  void test_get_value_template_data() const;

  void test_background() const;

  void test_delegate_template() const;
  void test_delegate_template_data() const;

  void test_no_set_value() const;

  void test_set_value_template() const;
  void test_set_value_template_data() const;

  void test_delete_cell_template();
  void test_delete_cell_template_data();

  void test_delete_3_groups_template();
  void test_delete_3_groups_template_data();

  void test_paste_cell_template();
  void test_paste_cell_template_data();

  void test_cut_paste_cell_template();
  void test_cut_paste_cell_template_data();

  void test_paste_3_groups_template();
  void test_paste_3_groups_template_data();

  void test_paste_truncate_template();
  void test_paste_truncate_template_data();

  void test_paste_recycle_template();
  void test_paste_recycle_template_data();

  void test_insert_into() const;
  void test_new_chord_from_chord() const;
  void test_new_note_from_chord() const;
  void test_new_note_from_note() const;

  void test_insert_rows_template();
  void test_insert_rows_template_data();

  void test_delete_rows_template();
  void test_delete_rows_template_data();

  void test_paste_rows_template();
  void test_paste_rows_template_data();

  void test_bad_paste_template();
  void test_bad_paste_template_data();

  void test_paste_wrong_level_template();
  void test_paste_wrong_level_template_data();

  void test_paste_into();

  void test_too_loud();
  void test_too_many_channels();

  void test_play_template() const;
  void test_play_template_data() const;

  void test_expand_collapse() const;

  void test_file_dialog();

  void test_save();
  void test_export();
  void test_broken_file_template();
  static void test_broken_file_template_data();
  void test_open_empty();
};
