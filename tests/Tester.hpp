#pragma once

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QStandardPaths>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/QtAssert>
#include <QtCore/qobjectdefs.h>
#include <QtCore/qtmetamacros.h>
#include <QtGui/QClipboard>
#include <QtTest/qtestcase.h>
#include <QtWidgets/QMessageBox>

#include "other/helpers.hpp"
#include "test_helpers.hpp"
#include "widgets/SongEditor.hpp"

struct Tester : public QObject {
  Q_OBJECT
public:
  SongEditor song_editor;
  QDir test_dir = get_share_folder();
  bool waiting_for_message = false;
  // watches for a QMessageBox that pops up while no test is expecting one
  // (via close_message_later) and fails fast instead of leaving it open --
  // QMessageBox::exec() runs a nested event loop, so this timer still fires
  // while a test is blocked waiting on it
  QTimer unexpected_message_timer;

  Tester() {
    // redirects QStandardPaths::AppDataLocation and QSettings's default
    // storage (used by the crash-recovery feature) away from the real
    // per-user Justly config/data dirs, so running tests can't clobber a
    // recovery file left behind by a real crashed session
    QStandardPaths::setTestModeEnabled(true);
    set_up();
    const auto fixture_file = test_dir.filePath("test_song.xml");
    // fail fast here instead of letting open_file's "Invalid XML file"
    // QMessageBox block forever with no user around to dismiss it
    Q_ASSERT(QFile::exists(fixture_file));
    open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                         song_editor.piano_roll_widget, fixture_file);

    QObject::connect(&unexpected_message_timer, &QTimer::timeout, this,
                     [this]() -> auto {
                       if (waiting_for_message) {
                         return;
                       }
                       auto *const box_pointer = find_top_level_message_box();
                       if (box_pointer == nullptr) {
                         return;
                       }
                       const auto actual_text = box_pointer->text();
                       box_pointer->close();
                       QFAIL(qUtf8Printable(
                           QString("Unexpected message box: %1")
                               .arg(actual_text)));
                     });
    unexpected_message_timer.start(WAIT_TIME);
  }

private slots:
  // the OS clipboard is a process-global singleton that outlives any single
  // test, so a stale copy left over from an earlier test (e.g. a copied
  // voice_number cell) must not bleed into a later test's voice removal and
  // trigger an unexpected clipboard-reassignment warning
  static void init() { get_clipboard().clear(); }

  static void test_column_count_data();
  void test_column_count();
  static void test_column_header_data();
  void test_column_header();
  static void test_copy_data();
  void test_copy();
  static void test_cut_data();
  void test_cut();
  static void test_delete_data();
  void test_delete();
  void test_export();
  void test_export_midi();
  void test_export_via_dialog();
  void test_export_midi_via_dialog();
  void test_export_unwritable_path();
  void test_file_dialog_cleanup();
  static void test_midi_append_variable_length_data();
  static void test_midi_append_variable_length();
  static void test_midi_byte_encoding();
  static void test_midi_track_event_write_dispatch();
  static void test_flag_data();
  void test_flag();
  void test_frequency_bound_data();
  void test_frequency_bound();
  static void test_frequency_in_status_data();
  void test_frequency_in_status();
  void test_gain();
  static void test_fluid_driver_move_assign();
  static void test_insert_after_data();
  void test_insert_after();
  static void test_insert_into_data();
  void test_insert_into();
  void test_interval_button_data();
  void test_interval_button();
  static void test_voice_error_data();
  void test_voice_error();
  static void test_voice_name_rejected_data();
  void test_voice_name_rejected();
  static void test_remove_voice_reassigns_notes_data();
  void test_remove_voice_reassigns_notes();
  void test_remove_voice_row_consistent_during_warning();
  static void test_remove_last_voice_disables_action_data();
  void test_remove_last_voice_disables_action();
  static void test_unreduced_ratio_from_xml_data();
  void test_unreduced_ratio_from_xml();
  static void test_interval_zero_numerator_does_not_hang();
  static void test_insert_xml_rows_respects_first_row_number();
  static void test_musicxml_data();
  void test_musicxml();
  static void test_musicxml_error_data();
  void test_musicxml_error();
  static void test_compute_measure_expansion_lone_backward_repeat();
  void test_import_musicxml_ties_do_not_cross_voices();
  void test_import_musicxml_orphan_tie_stop();
  static void test_zip_entry_size_is_safe_data();
  static void test_zip_entry_size_is_safe();
  void test_read_zip_entry_null_archive() const;
  static void test_xml_bytes_size_is_safe_data();
  static void test_xml_bytes_size_is_safe();
  static void test_string_to_maybe_int_data();
  static void test_string_to_maybe_int();
  void test_get_share_file_existing() const;
  void test_import_musicxml_after_editing_chord_notes();
  void test_open_after_editing_chord_notes_resets_menu();
  void test_open_action_enabled_outside_chords_view();
  void test_failed_import_does_not_reset_notes_view();
  static void test_next_previous_data();
  void test_next_previous();
  void test_reconnecting_selection_model_does_not_duplicate_connections();
  void test_octave_bound();
  static void test_open_error_data();
  void test_open_error();
  static void test_paste_after_data();
  void test_paste_after();
  static void test_paste_error_data();
  void test_paste_error();
  static void test_paste_into_data();
  void test_paste_into();
  static void test_paste_stale_voice_data();
  void test_paste_stale_voice();
  static void test_paste_voice_renumbered_on_insert_data();
  void test_paste_voice_renumbered_on_insert();
  static void test_paste_chord_voice_renumbered_on_insert_data();
  void test_paste_chord_voice_renumbered_on_insert();
  static void test_play_data();
  void test_play();
  void test_play_to_end_starts_playhead();
  static void test_play_to_end_data();
  void test_play_to_end();
  void test_ratio_bound_data();
  void test_ratio_bound();
  static void test_remove_row_data();
  void test_remove_row();
  void test_replace_table_combining();
  static void test_row_count_data();
  void test_row_count();
  static void test_row_header_data();
  void test_row_header();
  void test_save();
  void test_save_error_does_not_lose_work();
  void test_recovery_removed_on_save_and_open();
  void test_recovery_timer_debounce();
  void test_recovery_restore_accepted();
  void test_recovery_restore_declined();
  void test_recovery_no_prompt_when_missing();
  void test_starting_control_data();
  void test_starting_control();
  static void test_status_data();
  void test_status();
  static void test_voice_velocity_ratio_data();
  void test_voice_velocity_ratio();
  static void test_set_value_data();
  void test_set_value();
  static void test_set_voice_name_data();
  void test_set_voice_name();
  static void test_voice_paste_insert_disabled_data();
  void test_voice_paste_insert_disabled();
  static void test_to_string_data();
  void test_to_string();
  static void test_unused_role_data();
  void test_unused_role();
  static void test_piano_roll_events_data();
  void test_piano_roll_events() const;
  void test_piano_roll_events_total_count() const;
  void test_piano_roll_time_bounds() const;
  void test_piano_roll_dock_toggle();
  void test_piano_roll_rebuilds_on_edit();
  static void test_piano_roll_double_click_selects_note_data();
  void test_piano_roll_double_click_selects_note();
  static void test_piano_roll_click_selects_note_data();
  void test_piano_roll_click_selects_note();
  void test_piano_roll_notes_mode_shows_only_chord_notes();
  void test_piano_roll_notes_mode_axis_starts_at_chord_start();
  void test_piano_roll_selection_highlights_chord();
  static void test_piano_roll_selection_highlights_note_data();
  void test_piano_roll_selection_highlights_note();
  void test_piano_roll_selection_ignores_voice_table();
  void test_piano_roll_selection_preserves_multi_row_range();
  void test_piano_roll_drag_selects_chord();
  void test_piano_roll_drag_selects_chord_range();
  void test_piano_roll_playback_selects_chord();
  void test_piano_roll_zoom();
};