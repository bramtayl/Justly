

#include "Tester.hpp"

void Tester::test_voice_error_data() {
  QTest::addColumn<QString>("text");
  QTest::addColumn<QString>("error_message");

  static const QString header =
      "<song><gain>1</gain><starting_key>220</starting_key>"
      "<starting_tempo>100</starting_tempo><starting_velocity>10</"
      "starting_velocity>";
  static const QString one_pitched_voice =
      "<pitched_voices><pitched_voice><name>A</name>"
      "<instrument>Marimba</instrument></pitched_voice></pitched_voices>";
  static const QString one_unpitched_voice =
      "<unpitched_voices><unpitched_voice><name>B</name>"
      "<percussion_set_pointer>Room</percussion_set_pointer><midi_number>36</"
      "midi_number></unpitched_voice></unpitched_voices>";

  QTest::newRow("pitched voice number out of range")
      << header + one_pitched_voice + one_unpitched_voice +
             "<chords><chord><pitched_notes><pitched_note>"
             "<voice_number>5</voice_number></pitched_note></"
             "pitched_notes></chord></chords></song>"
      << "Voice 5 for chord 1, pitched note 1 has no corresponding voice";
  QTest::newRow("unpitched voice number out of range")
      << header + one_pitched_voice + one_unpitched_voice +
             "<chords><chord><unpitched_notes><unpitched_note>"
             "<voice_number>5</voice_number></unpitched_note></"
             "unpitched_notes></chord></chords></song>"
      << "Voice 5 for chord 1, unpitched note 1 has no corresponding voice";
  QTest::newRow("duplicate pitched voice name")
      << header +
             "<pitched_voices><pitched_voice><name>A</name>"
             "<instrument>Marimba</instrument></pitched_voice>"
             "<pitched_voice><name>A</name><instrument>Grand Piano</"
             "instrument></pitched_voice></pitched_voices>" +
             one_unpitched_voice + "</song>"
      << "Duplicate voice name \"A\"!";
  QTest::newRow("duplicate unpitched voice name")
      << header + one_pitched_voice +
             "<unpitched_voices><unpitched_voice><name>B</name>"
             "<percussion_set_pointer>Room</percussion_set_pointer>"
             "<midi_number>36</midi_number></unpitched_voice>"
             "<unpitched_voice><name>B</name><percussion_set_pointer>Power</"
             "percussion_set_pointer><midi_number>37</midi_number></"
             "unpitched_voice></unpitched_voices></song>"
      << "Duplicate voice name \"B\"!";
  QTest::newRow("empty pitched voice name")
      << header +
             "<pitched_voices><pitched_voice><name></name>"
             "<instrument>Marimba</instrument></pitched_voice></"
             "pitched_voices>" +
             one_unpitched_voice + "</song>"
      << "Voice name is empty!";
  QTest::newRow("empty unpitched voice name")
      << header + one_pitched_voice +
             "<unpitched_voices><unpitched_voice><name></name>"
             "<percussion_set_pointer>Room</percussion_set_pointer>"
             "<midi_number>36</midi_number></unpitched_voice></"
             "unpitched_voices></song>"
      << "Voice name is empty!";
}

void Tester::test_voice_error() {
  QFETCH(const QString, text);
  QFETCH(const QString, error_message);

  auto& song_widget = song_editor.song_widget;
  auto& chords_model = song_widget.switch_column.switch_table.chords_model;
  const auto old_current_file = song_widget.current_file;
  const auto old_chord_count = chords_model.rowCount(QModelIndex());
  QVERIFY(old_chord_count > 0);

  close_message_later(song_editor, waiting_for_message, error_message);
  open_text(song_editor, text);

  // a file that fails voice validation must leave the previously open
  // song untouched rather than clearing it out from under the user (open_file
  // used to clear/repopulate the models before validating, so a rejected
  // file silently wiped out whatever was open, with no undo path back)
  QCOMPARE(song_widget.current_file, old_current_file);
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

  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& undo_stack = song_widget.undo_stack;

  switch_to(song_editor, row_type, chord_number);

  auto& model = get_model(switch_table);
  const auto test_index = model.index(0, column_number);
  const auto old_value = test_index.data();

  close_message_later(song_editor, waiting_for_message, warning_message);
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

  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& undo_stack = song_widget.undo_stack;
  auto& song = song_widget.song;
  const auto voice_row_type =
      is_pitched ? RowType::pitched_voice_type : RowType::unpitched_voice_type;

  open_text(song_editor, text);

  switch_to(song_editor, voice_row_type, -1);
  QCOMPARE(get_model(switch_table).rowCount(), 3);
  select_cell(switch_table, 1, 0);
  close_message_later(song_editor, waiting_for_message, warning_message);
  song_editor.song_menu_bar.edit_menu.remove_rows_action.trigger();
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
  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget,
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
  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& undo_stack = song_widget.undo_stack;
  auto& song = song_widget.song;

  open_text(song_editor,
            make_voice_song_xml({"A", "B", "C"}, {"D"}, {{{0, 1, 2}, {}}}));

  switch_to(song_editor, RowType::pitched_voice_type, -1);
  select_cell(switch_table, 1, 0);

  const auto waiting_before = waiting_for_message;
  waiting_for_message = true;
  auto& timer =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QTimer(&song_editor));
  timer.setSingleShot(true);
  QObject::connect(
      &timer, &QTimer::timeout, &song_editor, [this, &song]() -> auto {
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

  song_editor.song_menu_bar.edit_menu.remove_rows_action.trigger();
  QVERIFY(!waiting_for_message);

  QCOMPARE(song.pitched_voices.size(), 2);
  const auto& notes = song.chords.at(0).pitched_notes;
  QCOMPARE(notes.at(0).voice_number, 0);
  QCOMPARE(notes.at(1).voice_number, 0);
  QCOMPARE(notes.at(2).voice_number, 1);

  undo_stack.undo();  // undo the voice removal
  maybe_switch_back_to_chords(undo_stack, RowType::pitched_voice_type);

  // restore the shared fixture
  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget,
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

  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& undo_stack = song_widget.undo_stack;
  const auto voice_row_type =
      is_pitched ? RowType::pitched_voice_type : RowType::unpitched_voice_type;

  open_text(song_editor, text);

  switch_to(song_editor, voice_row_type, -1);
  auto& model = get_model(switch_table);
  QCOMPARE(model.rowCount(), 1);

  select_cell(switch_table, 0, 0);
  QVERIFY(!song_editor.song_menu_bar.edit_menu.remove_rows_action.isEnabled());

  maybe_switch_back_to_chords(undo_stack, voice_row_type);

  // restore the shared fixture
  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget,
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
  // deleted before the paste happens, the clipboard's voice_number must be
  // reassigned to the first remaining voice the same way the live note is,
  // so pasting lands on a valid voice instead of indexing into the voices
  // list with the now out-of-range voice number
  QFETCH(const QString, text);
  QFETCH(const bool, is_pitched);

  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& edit_menu = song_editor.song_menu_bar.edit_menu;
  auto& back_to_chords_action =
      song_editor.song_menu_bar.view_menu.back_to_chords_action;
  auto& song = song_widget.song;

  const auto note_row_type =
      is_pitched ? RowType::pitched_note_type : RowType::unpitched_note_type;
  const auto voice_row_type =
      is_pitched ? RowType::pitched_voice_type : RowType::unpitched_voice_type;
  const auto voice_column =
      is_pitched ? static_cast<int>(
                       PitchedNoteColumn::pitched_note_voice_number_column)
                 : static_cast<int>(
                       UnpitchedNoteColumn::unpitched_note_voice_number_column);
  const QList<QString> reassign_warnings =
      is_pitched
          ? QList<QString>{"Reassigning 1 pitched note voice to the "
                           "first voice \"A\"",
                           "Reassigning 1 clipboard pitched note voice to "
                           "the first voice \"A\""}
          : QList<QString>{"Reassigning 1 unpitched note voice to the "
                           "first voice \"D\"",
                           "Reassigning 1 clipboard unpitched note voice "
                           "to the first voice \"D\""};

  open_text(song_editor, text);

  // copy the second note's voice cell, which references the last (soon to
  // be removed) voice
  switch_to(song_editor, note_row_type, 0);
  select_cell(switch_table, 1, voice_column);
  edit_menu.copy_action.trigger();
  back_to_chords_action.trigger();

  // remove that voice; the fixture's only note gets reassigned to voice 0,
  // warning about that live note, and separately about the clipboard copy
  // (of that same note's voice cell) that also collapses to voice 0
  switch_to(song_editor, voice_row_type, -1);
  select_cell(switch_table, 1, 0);
  close_messages_later(song_editor, waiting_for_message, reassign_warnings);
  edit_menu.remove_rows_action.trigger();
  back_to_chords_action.trigger();

  // pasting the reassigned clipboard should succeed silently, landing on
  // the first remaining voice instead of erroring or misassigning
  switch_to(song_editor, note_row_type, 0);
  select_cell(switch_table, 0, voice_column);
  edit_menu.paste_menu.paste_over_action.trigger();
  QCOMPARE(is_pitched ? song.chords.at(0).pitched_notes.at(0).voice_number
                      : song.chords.at(0).unpitched_notes.at(0).voice_number,
           0);
  back_to_chords_action.trigger();

  // restore the shared fixture
  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

void Tester::test_paste_voice_renumbered_on_insert_data() {
  QTest::addColumn<QString>("text");
  QTest::addColumn<bool>("is_pitched");

  static const QString pitched_song =
      make_voice_song_xml({"A", "B"}, {"D"}, {{{0, 1}, {}}});
  static const QString unpitched_song =
      make_voice_song_xml({"A"}, {"D", "E"}, {{{}, {0, 1}}});

  QTest::newRow("pitched voice") << pitched_song << true;
  QTest::newRow("unpitched voice") << unpitched_song << false;
}

void Tester::test_paste_voice_renumbered_on_insert() {
  // if the clipboard holds a note referencing a voice, and a voice is
  // inserted before it, the clipboard's voice_number must shift the same
  // way the live note's does, or pasting would silently land on whatever
  // voice now occupies the old index instead of the voice actually copied
  QFETCH(const QString, text);
  QFETCH(const bool, is_pitched);

  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& edit_menu = song_editor.song_menu_bar.edit_menu;
  auto& back_to_chords_action =
      song_editor.song_menu_bar.view_menu.back_to_chords_action;
  auto& song = song_widget.song;

  const auto note_row_type =
      is_pitched ? RowType::pitched_note_type : RowType::unpitched_note_type;
  const auto voice_row_type =
      is_pitched ? RowType::pitched_voice_type : RowType::unpitched_voice_type;
  const auto voice_column =
      is_pitched ? static_cast<int>(
                       PitchedNoteColumn::pitched_note_voice_number_column)
                 : static_cast<int>(
                       UnpitchedNoteColumn::unpitched_note_voice_number_column);

  open_text(song_editor, text);

  // copy the second note's voice cell, which references the second voice
  switch_to(song_editor, note_row_type, 0);
  select_cell(switch_table, 1, voice_column);
  edit_menu.copy_action.trigger();
  back_to_chords_action.trigger();

  // insert a new voice before both existing voices; the fixture's notes
  // both shift up by one, and so must the clipboard's copied voice_number
  switch_to(song_editor, voice_row_type, -1);
  select_cell(switch_table, 0, 0);
  edit_menu.insert_menu.insert_into_start_action.trigger();
  back_to_chords_action.trigger();

  // pasting the shifted clipboard should land on the second voice's new
  // index, not the stale pre-insert index (which now points elsewhere)
  switch_to(song_editor, note_row_type, 0);
  select_cell(switch_table, 0, voice_column);
  edit_menu.paste_menu.paste_over_action.trigger();
  QCOMPARE(is_pitched ? song.chords.at(0).pitched_notes.at(0).voice_number
                      : song.chords.at(0).unpitched_notes.at(0).voice_number,
           2);
  back_to_chords_action.trigger();

  // restore the shared fixture
  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

void Tester::test_paste_chord_voice_renumbered_on_insert_data() {
  QTest::addColumn<QString>("text");
  QTest::addColumn<bool>("is_pitched");

  static const QString pitched_song =
      make_voice_song_xml({"A", "B"}, {"D"}, {{{0, 1}, {}}});
  static const QString unpitched_song =
      make_voice_song_xml({"A"}, {"D", "E"}, {{{}, {0, 1}}});

  QTest::newRow("pitched voice") << pitched_song << true;
  QTest::newRow("unpitched voice") << unpitched_song << false;
}

void Tester::test_paste_chord_voice_renumbered_on_insert() {
  // copying a whole chord bakes its nested notes' voice_number into the
  // clipboard too; inserting a voice must shift those nested voice_numbers
  // the same way it shifts a flat note copy, or pasting the chord back
  // would silently restore the notes' stale, pre-insert voice numbers
  // (regression test for renumber_clipboard_voice_numbers not looking
  // inside a copied chord's nested pitched_notes/unpitched_notes)
  QFETCH(const QString, text);
  QFETCH(const bool, is_pitched);

  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& edit_menu = song_editor.song_menu_bar.edit_menu;
  auto& back_to_chords_action =
      song_editor.song_menu_bar.view_menu.back_to_chords_action;
  auto& song = song_widget.song;

  const auto voice_row_type =
      is_pitched ? RowType::pitched_voice_type : RowType::unpitched_voice_type;
  const auto last_column = Chord::get_number_of_columns() - 1;

  open_text(song_editor, text);

  // copy the whole chord row, including its pitched_notes/unpitched_notes
  // column, which nests both notes' voice_number fields in the clipboard
  get_selection_model(switch_table)
      .select(QItemSelection(get_model(switch_table).index(0, 0),
                             get_model(switch_table).index(0, last_column)),
              SELECT_AND_CLEAR);
  edit_menu.copy_action.trigger();

  // insert a new voice before both existing voices; the live chord's notes
  // shift up by one, and so must the nested voice_numbers on the clipboard
  switch_to(song_editor, voice_row_type, -1);
  select_cell(switch_table, 0, 0);
  edit_menu.insert_menu.insert_into_start_action.trigger();
  back_to_chords_action.trigger();

  // pasting the shifted clipboard back over the chord should restore both
  // notes at their new, shifted voice numbers, not the stale pre-insert ones
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
  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

void Tester::test_voice_velocity_ratio_data() {
  QTest::addColumn<QString>("text");
  QTest::addColumn<RowType>("row_type");
  QTest::addColumn<QString>("status");

  QTest::newRow("pitched voice velocity ratio")
      << QString(
             "<song><gain>1</gain><starting_key>220</starting_key>"
             "<starting_tempo>100</starting_tempo><starting_velocity>10</"
             "starting_velocity><pitched_voices><pitched_voice><name>A</"
             "name><instrument>Marimba</instrument><velocity_ratio>"
             "<numerator>2</numerator></velocity_ratio></pitched_voice></"
             "pitched_voices><unpitched_voices><unpitched_voice><name>D</"
             "name><percussion_set_pointer>Room</percussion_set_pointer>"
             "<midi_number>36</midi_number></unpitched_voice></"
             "unpitched_voices><chords><chord><pitched_notes>"
             "<pitched_note><voice_number>0</voice_number></pitched_note>"
             "</pitched_notes></chord></chords></song>")
      << RowType::pitched_note_type
      << "220 Hz ≈ A3; Velocity 20; 100 bpm; Start at 0 ms; Duration 600 ms";
  QTest::newRow("unpitched voice velocity ratio")
      << QString(
             "<song><gain>1</gain><starting_key>220</starting_key>"
             "<starting_tempo>100</starting_tempo><starting_velocity>10</"
             "starting_velocity><pitched_voices><pitched_voice><name>A</"
             "name><instrument>Marimba</instrument></pitched_voice></"
             "pitched_voices><unpitched_voices><unpitched_voice><name>D</"
             "name><percussion_set_pointer>Room</percussion_set_pointer>"
             "<midi_number>36</midi_number><velocity_ratio><numerator>2</"
             "numerator></velocity_ratio></unpitched_voice></"
             "unpitched_voices><chords><chord><unpitched_notes>"
             "<unpitched_note><voice_number>0</voice_number>"
             "</unpitched_note></unpitched_notes></chord></chords></song>")
      << RowType::unpitched_note_type
      << "Velocity 20; 100 bpm; Start at 0 ms; Duration 600 ms";
}

void Tester::test_voice_velocity_ratio() {
  // a voice's velocity ratio multiplies into the velocity of every note
  // that uses it, on top of the note's own separate velocity ratio
  QFETCH(const QString, text);
  QFETCH(const RowType, row_type);
  QFETCH(const QString, status);

  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;

  open_text(song_editor, text);

  switch_to(song_editor, row_type, 0);
  QCOMPARE(get_model(switch_table).index(0, 0).data(Qt::StatusTipRole), status);
  maybe_switch_back_to_chords(song_widget.undo_stack, row_type);

  // velocity ratio should also round-trip through save/load like any
  // other ratio column
  QTemporaryFile temp_save_file;
  QVERIFY(temp_save_file.open());
  temp_save_file.close();
  save_as_file(song_widget, temp_save_file.fileName());
  QVERIFY(get_file_text(temp_save_file.fileName())
              .contains("<velocity_ratio><numerator>2</numerator>"
                        "</velocity_ratio>"));
  QFile(temp_save_file.fileName()).remove();

  // restore the shared fixture
  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget,
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

  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& undo_stack = song_widget.undo_stack;

  switch_to(song_editor, row_type, -1);

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

  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& undo_stack = song_widget.undo_stack;
  auto& paste_menu = song_editor.song_menu_bar.edit_menu.paste_menu;

  switch_to(song_editor, row_type, -1);
  select_cell(switch_table, 0, column_number);

  QVERIFY(!paste_menu.paste_after_action.isEnabled());
  QVERIFY(!paste_menu.paste_into_start_action.isEnabled());

  maybe_switch_back_to_chords(undo_stack, row_type);
}
