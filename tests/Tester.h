#pragma once

#include <qabstractitemmodel.h>   // for QModelIndex
#include <qaction.h>              // for QAction
#include <qitemselectionmodel.h>  // for QItemSelectionModel
#include <qobject.h>              // for QObject
#include <qspinbox.h>
#include <qtemporaryfile.h>  // for QTemporaryFile
#include <qtmetamacros.h>    // for Q_OBJECT, slots
#include <qundostack.h>      // for QUndoStack

#include <string>  // for allocator, string

#include "justly/ChordsModel.hpp"       // for ChordsModel
#include "justly/ChordsView.hpp"        // for ChordsView
#include "justly/InstrumentEditor.hpp"  // for InstrumentEditor
#include "justly/SongEditor.hpp"        // for SongEditor

class Tester : public QObject {
  Q_OBJECT

  QTemporaryFile main_file;
  SongEditor song_editor;
  ChordsView* const chords_view_pointer = song_editor.chords_view_pointer;
  QUndoStack* const undo_stack_pointer = song_editor.undo_stack_pointer;
  InstrumentEditor* const starting_instrument_editor_pointer =
      song_editor.starting_instrument_editor_pointer;
  QDoubleSpinBox* const starting_key_editor_pointer =
      song_editor.starting_key_editor_pointer;
  QDoubleSpinBox* const starting_volume_editor_pointer =
      song_editor.starting_volume_editor_pointer;
  QDoubleSpinBox* const starting_tempo_editor_pointer =
      song_editor.starting_tempo_editor_pointer;
  QAction* const copy_action_pointer = song_editor.copy_action_pointer;
  QAction* const paste_before_action_pointer =
      song_editor.paste_before_action_pointer;
  QAction* const paste_after_action_pointer =
      song_editor.paste_after_action_pointer;
  QAction* const paste_into_action_pointer =
      song_editor.paste_into_action_pointer;
  QAction* const paste_cell_action_pointer =
      song_editor.paste_cell_action_pointer;
  QAction* const insert_before_action_pointer =
      song_editor.insert_before_action_pointer;
  QAction* const insert_after_action_pointer =
      song_editor.insert_after_action_pointer;
  QAction* const insert_into_action_pointer =
      song_editor.insert_into_action_pointer;
  QAction* const remove_action_pointer = song_editor.remove_action_pointer;
  QAction* const save_action_pointer = song_editor.save_action_pointer;
  QAction* const play_action_pointer = song_editor.play_action_pointer;
  QAction* const stop_playing_action_pointer =
      song_editor.stop_playing_action_pointer;

  QItemSelectionModel* const selector_pointer =
      chords_view_pointer->selectionModel();
  ChordsModel* const chords_model_pointer =
      chords_view_pointer->chords_model_pointer;

 private:
  bool waiting_for_message = false;
  void close_message_later(const std::string& expected_text);
  void clear_selection() const;
  void trigger_action(const QModelIndex& index,
                      QItemSelectionModel::SelectionFlags flags,
                      QAction* action_pointer) const;

 private slots:
  void initTestCase();
  static void test_interval();
  static void test_rational();

  void test_row_count_template();
  void test_row_count_template_data();

  void test_tree() const;

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

  void test_colors_template() const;
  void test_colors_template_data() const;

  void test_get_value_template() const;
  void test_get_value_template_data() const;

  void test_delegate_template() const;
  void test_delegate_template_data() const;

  void test_set_value() const;
  void test_set_value_template() const;
  void test_set_value_template_data() const;

  void test_paste_cell_template();
  void test_paste_cell_template_data();

  void test_paste_wrong_cell_template();
  void test_paste_wrong_cell_template_data();

  void test_insert_delete() const;

  void test_insert_delete_sibling_template();
  void test_insert_delete_sibling_template_data();

  void test_paste_siblings_template();
  void test_paste_siblings_template_data();

  void test_bad_paste_template();
  void test_bad_paste_template_data();
  
  void test_paste_rows();

  void test_play();
  void test_play_template() const;
  void test_play_template_data() const;

  void test_io();
};
