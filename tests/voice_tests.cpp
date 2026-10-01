

#include "Tester.hpp"

void Tester::test_voice_error_data() {
  QTest::addColumn<QString>("text");
  QTest::addColumn<QString>("error_message");

  QTest::newRow("unknown pitched voice name")
      << make_voice_song_xml({"A"}, {"B"}, {{{5}, {}}})
      << "Voice for chord 1, pitched note 1 has no corresponding voice";
  QTest::newRow("unknown unpitched voice name")
      << make_voice_song_xml({"A"}, {"B"}, {{{}, {5}}})
      << "Voice for chord 1, unpitched note 1 has no corresponding voice";
  // every note in a song file must name its voice
  QTest::newRow("pitched note without voice")
      << make_voice_song_xml({"A"}, {"B"}, {{{0}, {}}})
             .replace("<voice_name>A</voice_name>", "")
      << "Invalid song file";
  QTest::newRow("unpitched note without voice")
      << make_voice_song_xml({"A"}, {"B"}, {{{}, {0}}})
             .replace("<voice_name>B</voice_name>", "")
      << "Invalid song file";
  QTest::newRow("duplicate pitched voice name")
      << make_voice_song_xml({"A", "A"}, {"B"})
      << "Duplicate voice name \"A\"!";
  QTest::newRow("duplicate unpitched voice name")
      << make_voice_song_xml({"A"}, {"B", "B"})
      << "Duplicate voice name \"B\"!";
  QTest::newRow("empty pitched voice name")
      << make_voice_song_xml({""}, {"B"}) << "Voice name is empty!";
  QTest::newRow("empty unpitched voice name")
      << make_voice_song_xml({"A"}, {""}) << "Voice name is empty!";
}

void Tester::test_voice_error() {
  QFETCH(const QString, text);
  QFETCH(const QString, error_message);

  auto& window_body = main_window.window_body;
  auto& chords_model = window_body.switch_column.switch_table.chords_model;
  const auto old_current_file = window_body.current_file;
  const auto old_chord_count = chords_model.rowCount(QModelIndex());
  QVERIFY(old_chord_count > 0);

  close_message_later(main_window, waiting_for_message, error_message);
  open_text(main_window, text);

  // a file that fails voice validation must leave the previously open
  // song untouched rather than clearing it out from under the user (open_file
  // used to clear/repopulate the models before validating, so a rejected
  // file silently wiped out whatever was open, with no undo path back)
  QCOMPARE(window_body.current_file, old_current_file);
  QCOMPARE(chords_model.rowCount(QModelIndex()), old_chord_count);
}

void Tester::test_voice_name_rejected_data() {
  add_table_columns();
  QTest::addColumn<int>("column_number");
  QTest::addColumn<QVariant>("new_value");
  QTest::addColumn<QString>("warning_message");

  QTest::newRow("pitched voice duplicate name")
      << RowType::pitched_voice_type << -1
      << static_cast<int>(PitchedVoiceColumn::pitched_voice_name_column)
      << QVariant(QString("Guitar")) << "Voice \"Guitar\" already exists!";
  QTest::newRow("pitched voice empty name")
      << RowType::pitched_voice_type << -1
      << static_cast<int>(PitchedVoiceColumn::pitched_voice_name_column)
      << QVariant(QString()) << "Voice name is empty!";
  QTest::newRow("unpitched voice duplicate name")
      << RowType::unpitched_voice_type << -1
      << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_name_column)
      << QVariant(QString("Room Kit")) << "Voice \"Room Kit\" already exists!";
  QTest::newRow("unpitched voice empty name")
      << RowType::unpitched_voice_type << -1
      << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_name_column)
      << QVariant(QString()) << "Voice name is empty!";
}

void Tester::test_voice_name_rejected() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, column_number);
  QFETCH(const QVariant, new_value);
  QFETCH(const QString, warning_message);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& undo_stack = window_body.undo_stack;

  switch_to(main_window, row_type, chord_number);

  auto& model = get_model(switch_table);
  const auto test_index = model.index(0, column_number);
  const auto old_value = test_index.data();

  close_message_later(main_window, waiting_for_message, warning_message);
  QVERIFY(!model.setData(test_index, new_value, Qt::EditRole));
  QCOMPARE(test_index.data(), old_value);

  maybe_switch_back_to_chords(undo_stack, row_type);
}

void Tester::test_remove_voice_reassigns_notes_data() {
  QTest::addColumn<QString>("text");
  QTest::addColumn<bool>("is_pitched");
  QTest::addColumn<QString>("warning_message");

  static const QString pitched_song =
      make_voice_song_xml({"A", "B", "C"}, {"D"}, {{{0, 1, 2}, {}}});
  static const QString unpitched_song =
      make_voice_song_xml({"A"}, {"D", "E", "F"}, {{{}, {0, 1, 2}}});

  QTest::newRow("pitched voice")
      << pitched_song << true
      << "Reassigning 1 pitched note voice to the first voice \"A\"";
  QTest::newRow("unpitched voice")
      << unpitched_song << false
      << "Reassigning 1 unpitched note voice to the first voice \"D\"";
}

void Tester::test_remove_voice_reassigns_notes() {
  // removing voice 1 of 3 should: leave notes on voice 0 alone, reassign
  // (and warn about the first) note on the removed voice 1 to voice 0, and
  // shift notes on voice 2 down to voice 1
  QFETCH(const QString, text);
  QFETCH(const bool, is_pitched);
  QFETCH(const QString, warning_message);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& undo_stack = window_body.undo_stack;
  auto& song = window_body.song;
  const auto voice_row_type =
      is_pitched ? RowType::pitched_voice_type : RowType::unpitched_voice_type;

  open_text(main_window, text);

  switch_to(main_window, voice_row_type, -1);
  QCOMPARE(get_model(switch_table).rowCount(), 3);
  select_cell(switch_table, 1, 0);
  close_message_later(main_window, waiting_for_message, warning_message);
  main_window.song_menu_bar.edit_menu.remove_rows_action.trigger();
  QCOMPARE(get_model(switch_table).rowCount(), 2);

  if (is_pitched) {
    const auto& notes = song.chords.at(0).pitched_notes;
    QCOMPARE(song.pitched_voices.size(), 2);
    QCOMPARE(notes.at(0).voice_number, 0);
    QCOMPARE(notes.at(1).voice_number, 0);
    QCOMPARE(notes.at(2).voice_number, 1);
  } else {
    const auto& notes = song.chords.at(0).unpitched_notes;
    QCOMPARE(song.unpitched_voices.size(), 2);
    QCOMPARE(notes.at(0).voice_number, 0);
    QCOMPARE(notes.at(1).voice_number, 0);
    QCOMPARE(notes.at(2).voice_number, 1);
  }

  undo_stack.undo();  // undo the voice removal

  if (is_pitched) {
    const auto& notes = song.chords.at(0).pitched_notes;
    QCOMPARE(song.pitched_voices.size(), 3);
    QCOMPARE(notes.at(0).voice_number, 0);
    QCOMPARE(notes.at(1).voice_number, 1);
    QCOMPARE(notes.at(2).voice_number, 2);
  } else {
    const auto& notes = song.chords.at(0).unpitched_notes;
    QCOMPARE(song.unpitched_voices.size(), 3);
    QCOMPARE(notes.at(0).voice_number, 0);
    QCOMPARE(notes.at(1).voice_number, 1);
    QCOMPARE(notes.at(2).voice_number, 2);
  }

  maybe_switch_back_to_chords(undo_stack, voice_row_type);

  // restore the shared fixture
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

// regression test: RemoveVoiceRows::redo() used to shift note voice_numbers
// and remove the voice rows only *after* showing the "reassigned" warning
// dialog. QMessageBox::warning runs a nested event loop, so anything that
// repainted while that dialog was up would see a voices list that hadn't
// shrunk yet alongside notes already (or not yet) renumbered to match the
// post-removal state -- a transient mismatch. This checks that by the time
// the warning dialog appears, song.pitched_voices and every note's
// voice_number already agree with each other.
void Tester::test_remove_voice_row_consistent_during_warning() {
  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& undo_stack = window_body.undo_stack;
  auto& song = window_body.song;

  open_text(main_window,
            make_voice_song_xml({"A", "B", "C"}, {"D"}, {{{0, 1, 2}, {}}}));

  switch_to(main_window, RowType::pitched_voice_type, -1);
  select_cell(switch_table, 1, 0);

  const auto waiting_before = waiting_for_message;
  waiting_for_message = true;
  auto& timer =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QTimer(&main_window));
  timer.setSingleShot(true);
  QObject::connect(
      &timer, &QTimer::timeout, &main_window, [this, &song]() -> auto {
        auto* const box_pointer = find_top_level_message_box();
        if (box_pointer != nullptr) {
          waiting_for_message = false;
          QCOMPARE(song.pitched_voices.size(), 2);
          for (const auto& note : song.chords.at(0).pitched_notes) {
            QVERIFY(note.voice_number >= 0 &&
                    note.voice_number < song.pitched_voices.size());
          }
          QTest::keyEvent(QTest::Press, box_pointer, Qt::Key_Enter);
        }
      });
  timer.start(WAIT_TIME);
  QVERIFY(!waiting_before);

  main_window.song_menu_bar.edit_menu.remove_rows_action.trigger();
  QVERIFY(!waiting_for_message);

  QCOMPARE(song.pitched_voices.size(), 2);
  const auto& notes = song.chords.at(0).pitched_notes;
  QCOMPARE(notes.at(0).voice_number, 0);
  QCOMPARE(notes.at(1).voice_number, 0);
  QCOMPARE(notes.at(2).voice_number, 1);

  undo_stack.undo();  // undo the voice removal
  maybe_switch_back_to_chords(undo_stack, RowType::pitched_voice_type);

  // restore the shared fixture
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

void Tester::test_remove_last_voice_disables_action_data() {
  QTest::addColumn<QString>("text");
  QTest::addColumn<bool>("is_pitched");

  static const QString song_with_one_of_each_voice =
      make_voice_song_xml({"A"}, {"B"});

  QTest::newRow("pitched voice") << song_with_one_of_each_voice << true;
  QTest::newRow("unpitched voice") << song_with_one_of_each_voice << false;
}

void Tester::test_remove_last_voice_disables_action() {
  // selecting the only remaining voice of a type should disable
  // remove_rows_action, since at least one voice must always remain
  QFETCH(const QString, text);
  QFETCH(const bool, is_pitched);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& undo_stack = window_body.undo_stack;
  const auto voice_row_type =
      is_pitched ? RowType::pitched_voice_type : RowType::unpitched_voice_type;

  open_text(main_window, text);

  switch_to(main_window, voice_row_type, -1);
  auto& model = get_model(switch_table);
  QCOMPARE(model.rowCount(), 1);

  select_cell(switch_table, 0, 0);
  QVERIFY(!main_window.song_menu_bar.edit_menu.remove_rows_action.isEnabled());

  maybe_switch_back_to_chords(undo_stack, voice_row_type);

  // restore the shared fixture
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

void Tester::test_paste_stale_voice_data() {
  QTest::addColumn<QString>("text");
  QTest::addColumn<bool>("is_pitched");

  static const QString pitched_song =
      make_voice_song_xml({"A", "B"}, {"D"}, {{{0, 1}, {}}});
  static const QString unpitched_song =
      make_voice_song_xml({"A"}, {"D", "E"}, {{{}, {0, 1}}});

  QTest::newRow("pitched voice") << pitched_song << true;
  QTest::newRow("unpitched voice") << unpitched_song << false;
}

void Tester::test_paste_stale_voice() {
  // if the clipboard holds a note referencing a voice, and that voice gets
  // deleted before the paste happens, pasting reassigns it to the first
  // remaining voice the same way removing the voice reassigned the live note
  QFETCH(const QString, text);
  QFETCH(const bool, is_pitched);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& edit_menu = main_window.song_menu_bar.edit_menu;
  auto& back_to_chords_action =
      main_window.song_menu_bar.view_menu.back_to_chords_action;
  auto& song = window_body.song;

  const auto note_row_type =
      is_pitched ? RowType::pitched_note_type : RowType::unpitched_note_type;
  const auto voice_row_type =
      is_pitched ? RowType::pitched_voice_type : RowType::unpitched_voice_type;
  const auto voice_column =
      is_pitched ? static_cast<int>(
                       PitchedNoteColumn::pitched_note_voice_number_column)
                 : static_cast<int>(
                       UnpitchedNoteColumn::unpitched_note_voice_number_column);
  const auto* const remove_warning =
      is_pitched ? "Reassigning 1 pitched note voice to the first voice \"A\""
                 : "Reassigning 1 unpitched note voice to the first voice "
                   "\"D\"";
  const auto* const paste_warning =
      is_pitched ? "Reassigning 1 clipboard pitched note voice to the first "
                   "voice \"A\""
                 : "Reassigning 1 clipboard unpitched note voice to the first "
                   "voice \"D\"";

  open_text(main_window, text);

  // copy the second note's voice cell, which references the last (soon to
  // be removed) voice
  switch_to(main_window, note_row_type, 0);
  select_cell(switch_table, 1, voice_column);
  edit_menu.copy_action.trigger();
  back_to_chords_action.trigger();

  // remove that voice; the live note on it gets reassigned to voice 0
  switch_to(main_window, voice_row_type, -1);
  select_cell(switch_table, 1, 0);
  close_message_later(main_window, waiting_for_message, remove_warning);
  edit_menu.remove_rows_action.trigger();
  QVERIFY(!waiting_for_message);
  back_to_chords_action.trigger();

  // the clipboard still names the removed voice, so pasting lands on the
  // first remaining voice, with a warning
  switch_to(main_window, note_row_type, 0);
  select_cell(switch_table, 0, voice_column);
  close_message_later(main_window, waiting_for_message, paste_warning);
  edit_menu.paste_menu.paste_over_action.trigger();
  QVERIFY(!waiting_for_message);
  QCOMPARE(is_pitched ? song.chords.at(0).pitched_notes.at(0).voice_number
                      : song.chords.at(0).unpitched_notes.at(0).voice_number,
           0);
  back_to_chords_action.trigger();

  // restore the shared fixture
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

void Tester::test_paste_voice_after_insert_data() {
  QTest::addColumn<QString>("text");
  QTest::addColumn<bool>("is_pitched");

  static const QString pitched_song =
      make_voice_song_xml({"A", "B"}, {"D"}, {{{0, 1}, {}}});
  static const QString unpitched_song =
      make_voice_song_xml({"A"}, {"D", "E"}, {{{}, {0, 1}}});

  QTest::newRow("pitched voice") << pitched_song << true;
  QTest::newRow("unpitched voice") << unpitched_song << false;
}

void Tester::test_paste_voice_after_insert() {
  // copied notes name their voice, so inserting a voice before it doesn't
  // change which voice pasting lands on, even though its number shifts
  QFETCH(const QString, text);
  QFETCH(const bool, is_pitched);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& edit_menu = main_window.song_menu_bar.edit_menu;
  auto& back_to_chords_action =
      main_window.song_menu_bar.view_menu.back_to_chords_action;
  auto& song = window_body.song;

  const auto note_row_type =
      is_pitched ? RowType::pitched_note_type : RowType::unpitched_note_type;
  const auto voice_row_type =
      is_pitched ? RowType::pitched_voice_type : RowType::unpitched_voice_type;
  const auto voice_column =
      is_pitched ? static_cast<int>(
                       PitchedNoteColumn::pitched_note_voice_number_column)
                 : static_cast<int>(
                       UnpitchedNoteColumn::unpitched_note_voice_number_column);

  open_text(main_window, text);

  // copy the second note's voice cell, which references the second voice
  switch_to(main_window, note_row_type, 0);
  select_cell(switch_table, 1, voice_column);
  edit_menu.copy_action.trigger();
  back_to_chords_action.trigger();

  // insert a new voice before both existing voices, shifting their numbers
  switch_to(main_window, voice_row_type, -1);
  select_cell(switch_table, 0, 0);
  edit_menu.insert_menu.insert_into_start_action.trigger();
  back_to_chords_action.trigger();

  // pasting should land on the second voice's new number
  switch_to(main_window, note_row_type, 0);
  select_cell(switch_table, 0, voice_column);
  edit_menu.paste_menu.paste_over_action.trigger();
  QCOMPARE(is_pitched ? song.chords.at(0).pitched_notes.at(0).voice_number
                      : song.chords.at(0).unpitched_notes.at(0).voice_number,
           2);
  back_to_chords_action.trigger();

  // restore the shared fixture
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

void Tester::test_paste_chord_voice_after_insert_data() {
  QTest::addColumn<QString>("text");
  QTest::addColumn<bool>("is_pitched");

  static const QString pitched_song =
      make_voice_song_xml({"A", "B"}, {"D"}, {{{0, 1}, {}}});
  static const QString unpitched_song =
      make_voice_song_xml({"A"}, {"D", "E"}, {{{}, {0, 1}}});

  QTest::newRow("pitched voice") << pitched_song << true;
  QTest::newRow("unpitched voice") << unpitched_song << false;
}

void Tester::test_paste_chord_voice_after_insert() {
  // a copied chord's nested notes also name their voices, so inserting a
  // voice doesn't change which voices pasting the chord back lands on
  QFETCH(const QString, text);
  QFETCH(const bool, is_pitched);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& edit_menu = main_window.song_menu_bar.edit_menu;
  auto& back_to_chords_action =
      main_window.song_menu_bar.view_menu.back_to_chords_action;
  auto& song = window_body.song;

  const auto voice_row_type =
      is_pitched ? RowType::pitched_voice_type : RowType::unpitched_voice_type;
  const auto last_column = Chord::get_number_of_columns() - 1;

  open_text(main_window, text);

  // copy the whole chord row, including its pitched_notes/unpitched_notes
  // column, which nests both notes' voice names in the clipboard
  get_selection_model(switch_table)
      .select(QItemSelection(get_model(switch_table).index(0, 0),
                             get_model(switch_table).index(0, last_column)),
              SELECT_AND_CLEAR);
  edit_menu.copy_action.trigger();

  // insert a new voice before both existing voices, shifting their numbers
  switch_to(main_window, voice_row_type, -1);
  select_cell(switch_table, 0, 0);
  edit_menu.insert_menu.insert_into_start_action.trigger();
  back_to_chords_action.trigger();

  // pasting the chord back should restore both notes at their voices' new
  // numbers
  get_selection_model(switch_table)
      .select(QItemSelection(get_model(switch_table).index(0, 0),
                             get_model(switch_table).index(0, last_column)),
              SELECT_AND_CLEAR);
  edit_menu.paste_menu.paste_over_action.trigger();

  QCOMPARE(is_pitched ? song.chords.at(0).pitched_notes.at(0).voice_number
                      : song.chords.at(0).unpitched_notes.at(0).voice_number,
           1);
  QCOMPARE(is_pitched ? song.chords.at(0).pitched_notes.at(1).voice_number
                      : song.chords.at(0).unpitched_notes.at(1).voice_number,
           2);

  // restore the shared fixture
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

void Tester::test_voice_velocity_ratio_data() {
  QTest::addColumn<QString>("text");
  QTest::addColumn<RowType>("row_type");
  QTest::addColumn<QString>("status");

  QTest::newRow("pitched voice velocity ratio")
      << make_song_xml(R"(
  <pitched_voices>
    <pitched_voice>
      <name>A</name>
      <instrument>Marimba</instrument>
      <velocity_ratio><numerator>2</numerator></velocity_ratio>
    </pitched_voice>
  </pitched_voices>
  <unpitched_voices>
    <unpitched_voice>
      <name>D</name>
      <percussion_set_pointer>Room</percussion_set_pointer>
      <midi_number>36</midi_number>
    </unpitched_voice>
  </unpitched_voices>
  <chords>
    <chord>
      <pitched_notes>
        <pitched_note><voice_name>A</voice_name></pitched_note>
      </pitched_notes>
    </chord>
  </chords>)")
      << RowType::pitched_note_type
      << "220 Hz ≈ A3; Velocity 20; 100 bpm; Start at 0 ms; Duration 600 ms";
  QTest::newRow("unpitched voice velocity ratio")
      << make_song_xml(R"(
  <pitched_voices>
    <pitched_voice>
      <name>A</name>
      <instrument>Marimba</instrument>
    </pitched_voice>
  </pitched_voices>
  <unpitched_voices>
    <unpitched_voice>
      <name>D</name>
      <percussion_set_pointer>Room</percussion_set_pointer>
      <midi_number>36</midi_number>
      <velocity_ratio><numerator>2</numerator></velocity_ratio>
    </unpitched_voice>
  </unpitched_voices>
  <chords>
    <chord>
      <unpitched_notes>
        <unpitched_note><voice_name>D</voice_name></unpitched_note>
      </unpitched_notes>
    </chord>
  </chords>)")
      << RowType::unpitched_note_type
      << "Velocity 20; 100 bpm; Start at 0 ms; Duration 600 ms";
}

void Tester::test_voice_velocity_ratio() {
  // a voice's velocity ratio multiplies into the velocity of every note
  // that uses it, on top of the note's own separate velocity ratio
  QFETCH(const QString, text);
  QFETCH(const RowType, row_type);
  QFETCH(const QString, status);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;

  open_text(main_window, text);

  switch_to(main_window, row_type, 0);
  QCOMPARE(get_model(switch_table).index(0, 0).data(Qt::StatusTipRole), status);
  maybe_switch_back_to_chords(window_body.undo_stack, row_type);

  // velocity ratio should also round-trip through save/load like any
  // other ratio column
  QTemporaryFile temp_save_file;
  QVERIFY(temp_save_file.open());
  temp_save_file.close();
  save_as_file(window_body, temp_save_file.fileName());
  QVERIFY(get_file_text(temp_save_file.fileName())
              .contains("<velocity_ratio><numerator>2</numerator>"
                        "</velocity_ratio>"));
  QFile(temp_save_file.fileName()).remove();

  // restore the shared fixture
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

void Tester::test_set_voice_name_data() {
  QTest::addColumn<RowType>("row_type");
  QTest::addColumn<int>("column_number");
  QTest::addColumn<QString>("new_name");

  QTest::newRow("pitched voice")
      << RowType::pitched_voice_type
      << static_cast<int>(PitchedVoiceColumn::pitched_voice_name_column)
      << "New Pitched Voice Name";
  QTest::newRow("unpitched voice")
      << RowType::unpitched_voice_type
      << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_name_column)
      << "New Unpitched Voice Name";
}

void Tester::test_set_voice_name() {
  // unlike other voice columns, names must stay unique (see
  // check_voice_name in Voice.hpp), so this can't share test_set_value's
  // swap-two-existing-values pattern: setting a second row's name to a
  // first row's name would collide and warn
  QFETCH(const RowType, row_type);
  QFETCH(const int, column_number);
  QFETCH(const QString, new_name);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& undo_stack = window_body.undo_stack;

  switch_to(main_window, row_type, -1);

  auto& model = get_model(switch_table);
  const auto index = model.index(0, column_number);
  const auto old_value = index.data();
  QCOMPARE_NE(old_value.toString(), new_name);

  auto& delegate = get_reference(switch_table.itemDelegate());
  auto& cell_editor = get_reference(delegate.createEditor(
      &get_reference(switch_table.viewport()), QStyleOptionViewItem(), index));
  delegate.setEditorData(&cell_editor, index);
  cell_editor.setProperty(
      get_reference(cell_editor.metaObject()).userProperty().name(),
      QVariant(new_name));
  delegate.setModelData(&cell_editor, &model, index);

  QCOMPARE(index.data().toString(), new_name);
  undo_stack.undo();
  QCOMPARE(index.data(), old_value);

  maybe_switch_back_to_chords(undo_stack, row_type);
}

void Tester::test_voice_paste_insert_disabled_data() {
  QTest::addColumn<RowType>("row_type");
  QTest::addColumn<int>("column_number");

  QTest::newRow("pitched voice instrument")
      << RowType::pitched_voice_type
      << static_cast<int>(PitchedVoiceColumn::pitched_voice_instrument_column);
  QTest::newRow("unpitched voice percussion set")
      << RowType::unpitched_voice_type
      << static_cast<int>(
             UnpitchedVoiceColumn::unpitched_voice_percussion_set_column);
}

void Tester::test_voice_paste_insert_disabled() {
  // pasting after/into always inserts a brand new row built only from the
  // pasted column(s), which for voices would create one with an empty
  // (invalid) name -- see ReplaceTable.hpp
  QFETCH(const RowType, row_type);
  QFETCH(const int, column_number);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& undo_stack = window_body.undo_stack;
  auto& paste_menu = main_window.song_menu_bar.edit_menu.paste_menu;

  switch_to(main_window, row_type, -1);
  select_cell(switch_table, 0, column_number);

  QVERIFY(!paste_menu.paste_after_action.isEnabled());
  QVERIFY(!paste_menu.paste_into_start_action.isEnabled());

  maybe_switch_back_to_chords(undo_stack, row_type);
}

void Tester::test_voice_velocity_ratio_cells_data() {
  add_table_columns();
  QTest::addColumn<int>("column_number");

  QTest::newRow("pitched voice")
      << RowType::pitched_voice_type << -1
      << static_cast<int>(
             PitchedVoiceColumn::pitched_voice_velocity_ratio_column);
  QTest::newRow("unpitched voice")
      << RowType::unpitched_voice_type << -1
      << static_cast<int>(
             UnpitchedVoiceColumn::unpitched_voice_velocity_ratio_column);
}

void Tester::test_voice_velocity_ratio_cells() {
  // the shared fixture's voices all have the default velocity ratio, so the
  // generic set_value/copy tests (which need two distinct values) can't
  // cover this column -- set one voice's ratio, then copy/paste it onto the
  // other
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, column_number);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& edit_menu = main_window.song_menu_bar.edit_menu;

  open_text(main_window, make_voice_song_xml({"A", "B"}, {"C", "D"}));
  switch_to(main_window, row_type, chord_number);

  auto& model = get_model(switch_table);
  const auto first_index = model.index(0, column_number);
  const auto second_index = model.index(1, column_number);
  const auto new_ratio = QVariant::fromValue(Rational(2));

  QCOMPARE_NE(first_index.data(Qt::EditRole), new_ratio);
  QVERIFY(model.setData(first_index, new_ratio, Qt::EditRole));
  QCOMPARE(first_index.data(Qt::EditRole), new_ratio);

  select_cell(switch_table, 0, column_number);
  edit_menu.copy_action.trigger();
  select_cell(switch_table, 1, column_number);
  edit_menu.paste_menu.paste_over_action.trigger();
  QCOMPARE(second_index.data(Qt::EditRole), new_ratio);

  // restore the shared fixture
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

void Tester::test_paste_voice_after_remove_data() {
  QTest::addColumn<QString>("text");
  QTest::addColumn<bool>("is_pitched");

  static const QString pitched_song =
      make_voice_song_xml({"A", "B", "C"}, {"D"}, {{{1, 2}, {}}});
  static const QString unpitched_song =
      make_voice_song_xml({"A"}, {"D", "E", "F"}, {{{}, {1, 2}}});

  QTest::newRow("pitched voice") << pitched_song << true;
  QTest::newRow("unpitched voice") << unpitched_song << false;
}

void Tester::test_paste_voice_after_remove() {
  // copied notes name their voice, so removing an earlier voice doesn't
  // change which voice pasting lands on, even though its number shifts
  QFETCH(const QString, text);
  QFETCH(const bool, is_pitched);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& edit_menu = main_window.song_menu_bar.edit_menu;
  auto& back_to_chords_action =
      main_window.song_menu_bar.view_menu.back_to_chords_action;
  auto& song = window_body.song;

  const auto note_row_type =
      is_pitched ? RowType::pitched_note_type : RowType::unpitched_note_type;
  const auto voice_row_type =
      is_pitched ? RowType::pitched_voice_type : RowType::unpitched_voice_type;
  const auto voice_column =
      is_pitched ? static_cast<int>(
                       PitchedNoteColumn::pitched_note_voice_number_column)
                 : static_cast<int>(
                       UnpitchedNoteColumn::unpitched_note_voice_number_column);

  open_text(main_window, text);

  // copy the first note's voice cell, which references the second voice
  switch_to(main_window, note_row_type, 0);
  select_cell(switch_table, 0, voice_column);
  edit_menu.copy_action.trigger();
  back_to_chords_action.trigger();

  // remove the first voice; no note uses it, so nothing is reassigned or
  // warned about
  switch_to(main_window, voice_row_type, -1);
  select_cell(switch_table, 0, 0);
  edit_menu.remove_rows_action.trigger();
  back_to_chords_action.trigger();

  switch_to(main_window, note_row_type, 0);
  select_cell(switch_table, 1, voice_column);
  edit_menu.paste_menu.paste_over_action.trigger();
  QCOMPARE(is_pitched ? song.chords.at(0).pitched_notes.at(1).voice_number
                      : song.chords.at(0).unpitched_notes.at(1).voice_number,
           0);
  back_to_chords_action.trigger();

  // restore the shared fixture
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

void Tester::test_paste_unknown_voice_data() {
  QTest::addColumn<RowType>("row_type");
  QTest::addColumn<int>("chord_number");
  QTest::addColumn<QString>("copied");
  QTest::addColumn<QString>("mime_type");
  QTest::addColumn<QList<QString>>("warnings");

  // e.g. copied from another song
  QTest::newRow("pitched note")
      << RowType::pitched_note_type << 0
      << "<clipboard><left_column>0</left_column><right_column>0</"
         "right_column><rows><pitched_note><voice_name>Z</voice_name></"
         "pitched_note></rows></clipboard>"
      << PitchedNote::get_cells_mime()
      << QList<QString>{
             "Reassigning 1 clipboard pitched note voice to the first voice "
             "\"A\""};
  QTest::newRow("unpitched note")
      << RowType::unpitched_note_type << 0
      << "<clipboard><left_column>0</left_column><right_column>0</"
         "right_column><rows><unpitched_note><voice_name>Z</voice_name></"
         "unpitched_note></rows></clipboard>"
      << UnpitchedNote::get_cells_mime()
      << QList<QString>{
             "Reassigning 1 clipboard unpitched note voice to the first voice "
             "\"D\""};
  QTest::newRow("chord")
      << RowType::chord_type << -1
      << "<clipboard><left_column>0</left_column><right_column>1</"
         "right_column><rows><chord><pitched_notes><pitched_note><voice_name>"
         "Z</voice_name></pitched_note></pitched_notes><unpitched_notes>"
         "<unpitched_note><voice_name>Z</voice_name></unpitched_note>"
         "</unpitched_notes></chord></rows></clipboard>"
      << Chord::get_cells_mime()
      << QList<QString>{
             "Reassigning 1 clipboard pitched note voice to the first voice "
             "\"A\"",
             "Reassigning 1 clipboard unpitched note voice to the first voice "
             "\"D\""};
}

// pasted notes whose voice the song doesn't have land on the first voice,
// with a warning
void Tester::test_paste_unknown_voice() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const QString, copied);
  QFETCH(const QString, mime_type);
  QFETCH(const QList<QString>, warnings);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& song = window_body.song;

  open_text(main_window,
            make_voice_song_xml({"A", "B"}, {"D", "E"}, {{{1}, {1}}}));

  auto& new_data =
      get_reference(new QMimeData);  // NOLINT(cppcoreguidelines-owning-memory)
  new_data.setData(mime_type, copied.toUtf8());
  get_clipboard().setMimeData(&new_data);

  switch_to(main_window, row_type, chord_number);
  select_cell(switch_table, 0, 0);
  close_messages_later(main_window, waiting_for_message, warnings);
  main_window.song_menu_bar.edit_menu.paste_menu.paste_over_action.trigger();
  QVERIFY(!waiting_for_message);

  const auto& chord = song.chords.at(0);
  if (row_type != RowType::unpitched_note_type) {
    QCOMPARE(chord.pitched_notes.at(0).voice_number, 0);
  }
  if (row_type != RowType::pitched_note_type) {
    QCOMPARE(chord.unpitched_notes.at(0).voice_number, 0);
  }
  if (row_type != RowType::chord_type) {
    main_window.song_menu_bar.view_menu.back_to_chords_action.trigger();
  }

  // restore the shared fixture
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}
