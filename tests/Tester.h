#pragma once

#include <QItemSelectionModel>
#include <QObject>
#include <QString>

#include "justly/ChordsView.hpp"
#include "justly/SongEditor.hpp"

class ChordsModel;
class InstrumentEditor;
class QAction;
class QDoubleSpinBox;
class QModelIndex;
class QUndoStack;

class Tester : public QObject {
  Q_OBJECT

private:
  SongEditor song_editor;
  ChordsView *const chords_view_pointer = song_editor.chords_view_pointer;
  QUndoStack *const undo_stack_pointer = song_editor.undo_stack_pointer;
  InstrumentEditor *const starting_instrument_editor_pointer =
      song_editor.starting_instrument_editor_pointer;
  QDoubleSpinBox *const starting_key_editor_pointer =
      song_editor.starting_key_editor_pointer;
  QDoubleSpinBox *const starting_volume_percent_editor_pointer =
      song_editor.starting_volume_percent_editor_pointer;
  QDoubleSpinBox *const starting_tempo_editor_pointer =
      song_editor.starting_tempo_editor_pointer;
  QAction *const copy_action_pointer = song_editor.copy_action_pointer;
  QAction *const paste_cells_or_after_action_pointer =
      song_editor.paste_cells_or_after_action_pointer;
  QAction *const paste_into_action_pointer =
      song_editor.paste_into_action_pointer;
  QAction *const insert_after_action_pointer =
      song_editor.insert_after_action_pointer;
  QAction *const insert_into_action_pointer =
      song_editor.insert_into_action_pointer;
  QAction *const delete_action_pointer = song_editor.delete_action_pointer;
  QAction *const save_action_pointer = song_editor.save_action_pointer;
  QAction *const play_action_pointer = song_editor.play_action_pointer;
  QAction *const stop_playing_action_pointer =
      song_editor.stop_playing_action_pointer;

  QItemSelectionModel *const selector_pointer =
      chords_view_pointer->selectionModel();
  ChordsModel *const chords_model_pointer =
      chords_view_pointer->chords_model_pointer;

  bool waiting_for_message = false;
  void close_message_later(const QString &expected_text);
  void clear_selection() const;
  void trigger_action(const QModelIndex &index,
                      QItemSelectionModel::SelectionFlags flags,
                      QAction *action_pointer) const;

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

  void test_paste_wrong_cell_template();
  void test_paste_wrong_cell_template_data();

  void test_paste_too_many_template();
  void test_paste_too_many_template_data();

  void test_insert_delete() const;
  void test_insert_delete_template();
  void test_insert_delete_template_data();

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
