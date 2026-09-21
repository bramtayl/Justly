#include "Tester.hpp"
#include "widgets/piano_roll/PianoRollNotesScene.hpp"
#include "widgets/piano_roll/PianoRollWidget.hpp"

void Tester::test_play_data() {
  add_table_columns();
  QTest::addColumn<int>("first_row_number");
  QTest::addColumn<int>("second_row_number");
  QTest::addColumn<int>("column_number");

  QTest::newRow("two chords")
      << RowType::chord_type << -1 << 0 << 1
      << static_cast<int>(ChordColumn::chord_interval_column);
  QTest::newRow("one chord")
      << RowType::chord_type << -1 << 1 << 1
      << static_cast<int>(ChordColumn::chord_interval_column);
  QTest::newRow("two pitched notes")
      << RowType::pitched_note_type << 1 << 0 << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_voice_number_column);
  QTest::newRow("one pitched note")
      << RowType::pitched_note_type << 1 << 1 << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_voice_number_column);
  QTest::newRow("two unpitched notes")
      << RowType::unpitched_note_type << 1 << 0 << 1
      << static_cast<int>(
             UnpitchedNoteColumn::unpitched_note_voice_number_column);
  QTest::newRow("one unpitched note")
      << RowType::unpitched_note_type << 1 << 1 << 1
      << static_cast<int>(
             UnpitchedNoteColumn::unpitched_note_voice_number_column);
  QTest::newRow("two pitched voices")
      << RowType::pitched_voice_type << -1 << 0 << 1
      << static_cast<int>(PitchedVoiceColumn::pitched_voice_instrument_column);
  QTest::newRow("one pitched voice")
      << RowType::pitched_voice_type << -1 << 1 << 1
      << static_cast<int>(PitchedVoiceColumn::pitched_voice_instrument_column);
  QTest::newRow("two unpitched voices")
      << RowType::unpitched_voice_type << -1 << 0 << 1
      << static_cast<int>(
             UnpitchedVoiceColumn::unpitched_voice_midi_number_column);
  QTest::newRow("one unpitched voice")
      << RowType::unpitched_voice_type << -1 << 1 << 1
      << static_cast<int>(
             UnpitchedVoiceColumn::unpitched_voice_midi_number_column);
}

void Tester::test_play() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, first_row_number);
  QFETCH(const int, second_row_number);
  QFETCH(const int, column_number);

  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& play_menu = song_editor.song_menu_bar.play_menu;
  auto& play_action = play_menu.play_action;

  switch_to(song_editor, row_type, chord_number);

  auto& model = get_model(switch_table);
  get_selection_model(switch_table)
      .select(QItemSelection(model.index(first_row_number, column_number),
                             model.index(second_row_number, column_number)),
              SELECT_AND_CLEAR);
  play_action.trigger();
  // first cut off early
  play_action.trigger();
  // now play for a while
  QThread::msleep(WAIT_TIME);
  play_menu.stop_playing_action.trigger();

  maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
}

void Tester::test_play_to_end_starts_playhead() {
  // regression test: "Play to end" is a separate action from "Play
  // selection" and must independently start the piano roll playhead
  // animation, not just trigger audio playback
  auto& song_widget = song_editor.song_widget;
  auto& piano_roll_widget = song_editor.piano_roll_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& play_menu = song_editor.song_menu_bar.play_menu;

  select_cell(switch_table, 0, 0);

  QVERIFY(!piano_roll_widget.piano_roll_scene.playhead_active);
  play_menu.play_to_end_action.trigger();
  QVERIFY(piano_roll_widget.piano_roll_scene.playhead_active);

  QThread::msleep(WAIT_TIME);
  play_menu.stop_playing_action.trigger();
  QVERIFY(!piano_roll_widget.piano_roll_scene.playhead_active);
}

// unlike test_play, only exercises chord_type/pitched_note_type/
// unpitched_note_type -- play_to_end_action has no voice_type branch, it
// Q_ASSERTs false for anything else
void Tester::test_play_to_end_data() {
  add_table_columns();
  QTest::addColumn<int>("row_number");
  QTest::addColumn<int>("column_number");

  QTest::newRow("chord") << RowType::chord_type << -1 << 1
                         << static_cast<int>(
                                ChordColumn::chord_interval_column);
  QTest::newRow("pitched note")
      << RowType::pitched_note_type << 1 << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_voice_number_column);
  QTest::newRow("unpitched note")
      << RowType::unpitched_note_type << 1 << 1
      << static_cast<int>(
             UnpitchedNoteColumn::unpitched_note_voice_number_column);
}

void Tester::test_play_to_end() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, row_number);
  QFETCH(const int, column_number);

  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& play_menu = song_editor.song_menu_bar.play_menu;

  switch_to(song_editor, row_type, chord_number);

  select_cell(switch_table, row_number, column_number);
  play_menu.play_to_end_action.trigger();
  QThread::msleep(WAIT_TIME);
  play_menu.stop_playing_action.trigger();

  maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
}

// starting velocity 100 with a 2x note and/or voice velocity ratio exceeds
// MAX_VELOCITY (127), so playback must warn and abort instead of clamping
void Tester::test_play_velocity_error_data() {
  add_table_columns();
  QTest::addColumn<bool>("play_to_end");
  QTest::addColumn<QString>("error_message");

  QTest::newRow("play pitched note")
      << RowType::pitched_note_type << 0 << false
      << "Velocity 200 exceeds 127 for chord 1, pitched note 1";
  QTest::newRow("play unpitched note")
      << RowType::unpitched_note_type << 0 << false
      << "Velocity 200 exceeds 127 for chord 1, unpitched note 1";
  QTest::newRow("play pitched voice")
      << RowType::pitched_voice_type << -1 << false
      << "Velocity 200 exceeds 127 for pitched voice \"A\"";
  QTest::newRow("play unpitched voice")
      << RowType::unpitched_voice_type << -1 << false
      << "Velocity 200 exceeds 127 for unpitched voice \"D\"";
  QTest::newRow("play pitched note to end")
      << RowType::pitched_note_type << 0 << true
      << "Velocity 200 exceeds 127 for chord 1, pitched note 1";
  QTest::newRow("play unpitched note to end")
      << RowType::unpitched_note_type << 0 << true
      << "Velocity 200 exceeds 127 for chord 1, unpitched note 1";
}

void Tester::test_play_velocity_error() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const bool, play_to_end);
  QFETCH(const QString, error_message);

  static const QString song_text =
      "<song><gain>1</gain><starting_key>220</starting_key>"
      "<starting_tempo>100</starting_tempo><starting_velocity>100</"
      "starting_velocity><pitched_voices><pitched_voice><name>A</name>"
      "<instrument>Marimba</instrument><velocity_ratio><numerator>2</"
      "numerator></velocity_ratio></pitched_voice></pitched_voices>"
      "<unpitched_voices><unpitched_voice><name>D</name>"
      "<percussion_set_pointer>Room</percussion_set_pointer>"
      "<midi_number>36</midi_number><velocity_ratio><numerator>2</numerator>"
      "</velocity_ratio></unpitched_voice></unpitched_voices><chords><chord>"
      "<pitched_notes><pitched_note><voice_number>0</voice_number>"
      "</pitched_note></pitched_notes><unpitched_notes><unpitched_note>"
      "<voice_number>0</voice_number></unpitched_note></unpitched_notes>"
      "</chord></chords></song>";

  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& play_menu = song_editor.song_menu_bar.play_menu;

  open_text(song_editor, song_text);
  switch_to(song_editor, row_type, chord_number);
  select_cell(switch_table, 0, 0);

  close_message_later(song_editor, waiting_for_message, error_message);
  (play_to_end ? play_menu.play_to_end_action : play_menu.play_action)
      .trigger();
  play_menu.stop_playing_action.trigger();

  maybe_switch_back_to_chords(song_widget.undo_stack, row_type);

  // restore the shared fixture
  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

// every voice preview sounds for a second, so previewing more voices at once
// than there are MIDI channels leaves the last one with no free channel
void Tester::test_play_channel_exhausted() {
  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& play_menu = song_editor.song_menu_bar.play_menu;

  QList<QString> voice_names;
  for (auto voice_number = 0; voice_number <= NUMBER_OF_MIDI_CHANNELS;
       voice_number++) {
    voice_names.push_back(QString("Voice %1").arg(voice_number));
  }
  open_text(song_editor, make_voice_song_xml(voice_names, {"D"}));

  switch_to(song_editor, RowType::pitched_voice_type, -1);
  auto& model = get_model(switch_table);
  get_selection_model(switch_table)
      .select(QItemSelection(model.index(0, 0),
                             model.index(NUMBER_OF_MIDI_CHANNELS, 0)),
              SELECT_AND_CLEAR);

  close_message_later(song_editor, waiting_for_message,
                      "More notes are sounding at once than there are "
                      "available MIDI channels");
  play_menu.play_action.trigger();
  play_menu.stop_playing_action.trigger();

  maybe_switch_back_to_chords(song_widget.undo_stack,
                              RowType::pitched_voice_type);

  // restore the shared fixture
  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}
