#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLabel>

#include "Tester.hpp"
#include "musicxml/MusicXMLPart.hpp"
#include "widgets/ControlsColumn.hpp"
#include "widgets/SpinBoxes.hpp"

void Tester::test_musicxml_data() {
  QTest::addColumn<QString>("file_name");
  QTest::addColumn<int>("number_of_chords");

  QTest::newRow("prelude") << "prelude.musicxml" << MUSIC_XML_ROWS;
  QTest::newRow("compressed prelude") << "prelude.mxl" << MUSIC_XML_ROWS;
  QTest::newRow("percussion") << "percussion.musicxml" << PERCUSSION_ROWS;
  QTest::newRow("transposing instruments")
      << "MozartTrio.musicxml" << MOZART_ROWS;
  QTest::newRow("repeats") << "Saltarello.musicxml" << SALTARELLO_ROWS;
}

void Tester::test_musicxml() {
  QFETCH(const QString, file_name);
  QFETCH(const int, number_of_chords);

  auto& window_body = main_window.window_body;

  import_musicxml_and_reload(main_window.song_menu_bar, main_window.window_body,
                             main_window.piano_roll_widget,
                             test_dir.filePath(file_name));
  QCOMPARE(
      get_model(window_body.switch_column.switch_table).rowCount(QModelIndex()),
      number_of_chords);
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

void Tester::test_musicxml_error_data() {
  QTest::addColumn<QString>("file_name");
  QTest::addColumn<QString>("error_message");

  QTest::newRow("not musicxml")
      << "not_musicxml.xml" << "Invalid musicxml file";
  QTest::newRow("invalid mxl") << "invalid.mxl" << "Invalid XML file";
  // a compressed score's container.xml has to exist, parse, and name a
  // rootfile
  QTest::newRow("mxl without container")
      << "mxl_no_container.mxl" << "Invalid XML file";
  QTest::newRow("mxl with invalid container")
      << "mxl_bad_container.mxl" << "Invalid XML file";
  QTest::newRow("mxl container without rootfiles")
      << "mxl_no_rootfiles.mxl" << "Invalid XML file";
  QTest::newRow("mxl container without rootfile")
      << "mxl_no_rootfile.mxl" << "Invalid XML file";
  QTest::newRow("empty") << "empty.musicxml" << "No chords";
  QTest::newRow("grace notes") << "MozartPianoSonata.musicxml"
                               << "Notes without durations not supported";
  QTest::newRow("timewise") << "timewise.musicxml"
                            << "Justly only supports partwise musicxml scores";
  // regression test: divisions is xs:decimal in the musicxml schema (to
  // allow fractional divisions), but xml_to_int used to silently truncate
  // fractional content via std::stoi instead of rejecting it, corrupting
  // note timing with no warning
  QTest::newRow("fractional divisions")
      << "fractional_divisions.musicxml"
      << "Fractional divisions are not supported";
  // regression test: divisions is unbounded xs:decimal with no
  // schema-enforced range, so a magnitude that doesn't fit in a 32-bit int
  // must be rejected with a warning instead of overflowing std::stoi
  QTest::newRow("overflowing divisions") << "overflowing_divisions.musicxml"
                                         << "Divisions value is out of range";
}

void Tester::test_musicxml_error() {
  QFETCH(const QString, error_message);
  QFETCH(const QString, file_name);

  close_message_later(main_window, waiting_for_message, error_message);
  import_musicxml_and_reload(main_window.song_menu_bar, main_window.window_body,
                             main_window.piano_roll_widget,
                             test_dir.filePath(file_name));
}

// regression test: a backward repeat with no forward repeat since the
// last block boundary (e.g. a repeat that implicitly continues right
// after an earlier, already-expanded repeated section) must replay from
// that block boundary, not from measure 0 -- otherwise an earlier,
// already-consumed repeated section gets incorrectly replayed again, and
// the plain measures between the two sections get silently dropped
// instead of flushed
void Tester::test_playback_order_lone_backward_repeat() {
  QList<MusicXMLMeasure> measures(5);
  measures[0].has_forward_repeat = true;
  measures[1].has_backward_repeat = true;
  // a lone backward repeat: no forward repeat since measure 2
  measures[4].has_backward_repeat = true;

  const QList<int> expected_order = {
      0, 1, 0, 1,        // measures 0-1, twice
      2, 3, 4, 2, 3, 4,  // measures 2-4, twice
  };

  QCOMPARE(get_playback_order(measures), expected_order);
}

// regression test: the musicxml importer's in-progress-tie lookup used to
// be keyed by pitch alone, shared across every voice (and every part) in
// the whole document. Two simultaneous voices tying the same pitch (here,
// two instruments in one piano part) would clobber each other's still-open
// note: the second voice's tie-start silently overwrote the first voice's
// map entry, so the first voice's tie-stop resolved onto the wrong note
// (wrong words/duration) and the second voice's own tie-stop then found no
// entry at all. Keying the lookup by voice as well as pitch keeps
// overlapping ties on the same pitch independent.
void Tester::test_import_musicxml_ties_do_not_cross_voices() {
  auto& window_body = main_window.window_body;

  import_musicxml_and_reload(main_window.song_menu_bar, main_window.window_body,
                             main_window.piano_roll_widget,
                             test_dir.filePath("tied_voices.musicxml"));

  auto& song = window_body.song;
  QCOMPARE(song.chords.size(), 2);

  const auto& left_hand_notes = song.chords.at(0).pitched_notes;
  QCOMPARE(left_hand_notes.size(), 1);
  QCOMPARE(left_hand_notes.at(0).voice_name, song.pitched_voices.at(0).name);
  QVERIFY(left_hand_notes.at(0).words.contains("Left Hand"));
  QCOMPARE(left_hand_notes.at(0).beats.numerator, 8);
  QCOMPARE(left_hand_notes.at(0).beats.denominator, 1);

  const auto& right_hand_notes = song.chords.at(1).pitched_notes;
  QCOMPARE(right_hand_notes.size(), 1);
  QCOMPARE(right_hand_notes.at(0).voice_name, song.pitched_voices.at(1).name);
  QVERIFY(right_hand_notes.at(0).words.contains("Right Hand"));
  QCOMPARE(right_hand_notes.at(0).beats.numerator, 8);
  QCOMPARE(right_hand_notes.at(0).beats.denominator, 1);

  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

// regression test: a <tie type="stop"/> with no matching earlier
// <tie type="start"/> for the same pitch/voice used to dereference a
// missing map entry, guarded only by a release-mode-noop Q_ASSERT -- a
// malformed or hand-edited musicxml file could trigger undefined behavior.
// It must now import as an ordinary, unstarted note instead.
void Tester::test_import_musicxml_orphan_tie_stop() {
  auto& window_body = main_window.window_body;

  import_musicxml_and_reload(main_window.song_menu_bar, main_window.window_body,
                             main_window.piano_roll_widget,
                             test_dir.filePath("orphan_tie.musicxml"));

  auto& song = window_body.song;
  QCOMPARE(song.chords.size(), 1);

  const auto& notes = song.chords.at(0).pitched_notes;
  QCOMPARE(notes.size(), 1);
  QCOMPARE(notes.at(0).beats.numerator, 4);
  QCOMPARE(notes.at(0).beats.denominator, 1);

  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

// regression test: inserting a chord, then drilling into and inserting one
// of its notes, leaves the switch table's notes model pointing directly at
// that Chord's notes QList; import_musicxml/open_file must not crash even
// though they replace song.chords wholesale while that pointer is live
void Tester::test_import_musicxml_after_editing_chord_notes() {
  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& insert_menu = main_window.song_menu_bar.edit_menu.insert_menu;

  select_cell(switch_table, 0, 0);
  insert_menu.insert_after_action.trigger();
  switch_to(main_window, RowType::pitched_note_type, 1);
  insert_menu.insert_into_start_action.trigger();

  import_musicxml_and_reload(main_window.song_menu_bar, main_window.window_body,
                             main_window.piano_roll_widget,
                             test_dir.filePath("prelude.musicxml"));
  QCOMPARE(get_model(switch_table).rowCount(QModelIndex()), MUSIC_XML_ROWS);

  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

// regression test: opening/importing a file bypasses the undo stack, so
// replace_table (which normally resets the switch column's label and the
// view menu's actions whenever the table switches to a different row
// type) never runs for them; open_file_and_reload/import_musicxml_and_reload
// must reapply that same reset explicitly (via song_reloaded) instead of
// leaving the label/actions stuck showing whatever was being edited before
void Tester::test_open_after_editing_chord_notes_resets_menu() {
  auto& window_body = main_window.window_body;
  auto& switch_column = window_body.switch_column;
  auto& view_menu = main_window.song_menu_bar.view_menu;

  switch_to(main_window, RowType::pitched_note_type, 0);
  QCOMPARE(switch_column.editing_text.text(), "Pitched notes for chord 1");
  QVERIFY(view_menu.back_to_chords_action.isEnabled());

  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));

  QCOMPARE(switch_column.editing_text.text(), "Chords");
  QVERIFY(!view_menu.back_to_chords_action.isEnabled());
  QVERIFY(view_menu.edit_pitched_voices_action.isEnabled());
  QVERIFY(view_menu.edit_unpitched_voices_action.isEnabled());
  QVERIFY(!view_menu.previous_chord_action.isEnabled());
  QVERIFY(!view_menu.next_chord_action.isEnabled());
}

// regression test: File > Open used to be disabled while viewing anything
// other than chords (pitched/unpitched notes or voices), even though
// there's no reason opening a different file requires navigating back to
// the chords view first
void Tester::test_open_action_enabled_outside_chords_view() {
  auto& undo_stack = main_window.window_body.undo_stack;
  auto& open_action = main_window.song_menu_bar.file_menu.open_action;

  QVERIFY(open_action.isEnabled());

  switch_to(main_window, RowType::pitched_voice_type, -1);
  QVERIFY(open_action.isEnabled());
  maybe_switch_back_to_chords(undo_stack, RowType::pitched_voice_type);

  switch_to(main_window, RowType::pitched_note_type, 0);
  QVERIFY(open_action.isEnabled());
  maybe_switch_back_to_chords(undo_stack, RowType::pitched_note_type);
}

// regression test: song_reloaded (which resets the switch column's label/
// view menu and rebuilds the piano roll) must only run once open_file/
// import_musicxml actually succeed -- a rejected file must leave whatever
// view the user was on (including mid-note-editing) completely alone
// rather than silently bouncing them back to the chords view
void Tester::test_failed_import_does_not_reset_notes_view() {
  auto& window_body = main_window.window_body;
  auto& switch_column = window_body.switch_column;

  switch_to(main_window, RowType::pitched_note_type, 0);
  QCOMPARE(switch_column.editing_text.text(), "Pitched notes for chord 1");

  close_message_later(main_window, waiting_for_message,
                      "Invalid musicxml file");
  import_musicxml_and_reload(main_window.song_menu_bar, main_window.window_body,
                             main_window.piano_roll_widget,
                             test_dir.filePath("not_musicxml.xml"));

  QCOMPARE(switch_column.editing_text.text(), "Pitched notes for chord 1");

  maybe_switch_back_to_chords(window_body.undo_stack,
                              RowType::pitched_note_type);
}

// an imported voice whose name exactly matches a built-in program (here, the
// part is named "Marimba") should use that program instead of the default
void Tester::test_import_musicxml_voice_named_like_program() {
  auto& window_body = main_window.window_body;

  import_musicxml_and_reload(main_window.song_menu_bar, main_window.window_body,
                             main_window.piano_roll_widget,
                             test_dir.filePath("program_named_part.musicxml"));

  const auto& pitched_voices = window_body.song.pitched_voices;
  QCOMPARE(pitched_voices.size(), 1);
  QCOMPARE(pitched_voices.at(0).name, QString("Marimba"));
  QCOMPARE(pitched_voices.at(0).program, QString("Marimba"));

  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

namespace {

// wraps one measure of the given <attributes> children and body elements in
// a minimal single-part score
auto make_musicxml(const QString& attributes, const QString& body) -> QString {
  return QString(R"(
<score-partwise version="4.0">
  <part-list>
    <score-part id="P1"><part-name>P</part-name></score-part>
  </part-list>
  <part id="P1">
    <measure number="1">
      <attributes>%1</attributes>
      %2
    </measure>
  </part>
</score-partwise>)")
      .arg(attributes, body);
}

auto make_pitch_note(const QString& accidental, const QString& duration)
    -> QString {
  return "<note><pitch><step>C</step><octave>4</octave></pitch><duration>" +
         duration + "</duration>" +
         (accidental.isEmpty()
              ? QString()
              : "<accidental>" + accidental + "</accidental>") +
         "</note>";
}

// a quarter note (with one division per quarter), optionally tied, with an
// accidental, or on a given staff
auto make_spelled_note(const QString& step, const QString& octave,
                       const QString& accidental = "",
                       const QString& tie_type = "", const QString& staff = "")
    -> QString {
  return "<note><pitch><step>" + step + "</step><octave>" + octave +
         "</octave></pitch><duration>1</duration>" +
         (tie_type.isEmpty() ? QString()
                             : QString(R"(<tie type="%1"/>)").arg(tie_type)) +
         (accidental.isEmpty()
              ? QString()
              : "<accidental>" + accidental + "</accidental>") +
         (staff.isEmpty() ? QString() : "<staff>" + staff + "</staff>") +
         "</note>";
}

auto make_key(const QString& fifths) -> QString {
  return "<key><fifths>" + fifths + "</fifths></key>";
}

auto get_next_measure() -> QString {
  return R"(
    </measure>
    <measure number="2">)";
}

auto get_divisions() -> QString { return "<divisions>1</divisions>"; }
// too big for a 32-bit int, but still valid for the schema's unbounded types
auto get_huge_number() -> QString { return "99999999999"; }

}  // namespace

// each of these fields is unbounded or decimal in the musicxml schema, so a
// valid file can hold a value the importer can't represent; it must warn and
// reject the file instead of truncating or overflowing
void Tester::test_musicxml_inline_error_data() {
  QTest::addColumn<QString>("attributes");
  QTest::addColumn<QString>("body");
  QTest::addColumn<QString>("error_message");

  const auto plain_note = make_pitch_note("", "1");

  QTest::newRow("fifths out of range")
      << get_divisions() + "<key><fifths>" + get_huge_number() +
             "</fifths></key>"
      << plain_note << "Fifths value is out of range";
  QTest::newRow("microtonal transpose")
      << get_divisions() + "<transpose><chromatic>1.5</chromatic></transpose>"
      << plain_note << "Microtonal transpositions are not supported";
  QTest::newRow("chromatic out of range")
      << get_divisions() + "<transpose><chromatic>" + get_huge_number() +
             "</chromatic></transpose>"
      << plain_note << "Chromatic value is out of range";
  QTest::newRow("octave change out of range")
      << get_divisions() +
             "<transpose><chromatic>0</chromatic><octave-change>" +
             get_huge_number() + "</octave-change></transpose>"
      << plain_note << "Octave change value is out of range";
  // only arrows are read as septimal quartertones, since a bare quartertone
  // accidental doesn't say which chromatic accidental it modifies
  QTest::newRow("quartertone accidental")
      << get_divisions() << make_pitch_note("quarter-flat", "1")
      << "Accidental quarter-flat is not supported";
  QTest::newRow("other accidental")
      << get_divisions() << make_pitch_note("other", "1")
      << "Accidental other is not supported";
  QTest::newRow("fractional note duration")
      << get_divisions() << make_pitch_note("", "1.5")
      << "Fractional note durations are not supported";
  QTest::newRow("note duration out of range")
      << get_divisions() << make_pitch_note("", get_huge_number())
      << "Note duration is out of range";
  QTest::newRow("fractional backup")
      << get_divisions() << "<backup><duration>1.5</duration></backup>"
      << "Fractional durations are not supported";
  QTest::newRow("fractional forward")
      << get_divisions() << "<forward><duration>1.5</duration></forward>"
      << "Fractional durations are not supported";
  QTest::newRow("backup duration out of range")
      << get_divisions()
      << "<backup><duration>" + get_huge_number() + "</duration></backup>"
      << "Duration is out of range";
  QTest::newRow("forward duration out of range")
      << get_divisions()
      << "<forward><duration>" + get_huge_number() + "</duration></forward>"
      << "Duration is out of range";
  QTest::newRow("repeat times out of range")
      << get_divisions()
      << plain_note +
             QString(
                 R"(<barline><repeat direction="backward" times="%1"/></barline>)")
                 .arg(get_huge_number())
      << "Repeat times is out of range";
}

void Tester::test_musicxml_inline_error() {
  QFETCH(const QString, attributes);
  QFETCH(const QString, body);
  QFETCH(const QString, error_message);

  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write(make_musicxml(attributes, body).toStdString().c_str());
  temp_file.close();

  close_message_later(main_window, waiting_for_message, error_message);
  import_musicxml_and_reload(main_window.song_menu_bar, main_window.window_body,
                             main_window.piano_roll_widget,
                             temp_file.fileName());
}

// a backward repeat's explicit "times" attribute sets how many passes the
// repeated measure plays (the default, without it, is two)
void Tester::test_musicxml_repeat_times() {
  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write(
      make_musicxml(
          get_divisions(),
          make_pitch_note("", "1") +
              R"(<barline><repeat direction="backward" times="3"/></barline>)")
          .toStdString()
          .c_str());
  temp_file.close();

  auto& window_body = main_window.window_body;
  import_musicxml_and_reload(main_window.song_menu_bar, main_window.window_body,
                             main_window.piano_roll_widget,
                             temp_file.fileName());
  QCOMPARE(window_body.song.chords.size(), 3);

  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

// a transposing part's <octave-change> shifts every pitched note by that many
// octaves, on top of its chromatic transposition
void Tester::test_musicxml_octave_change() {
  auto& window_body = main_window.window_body;

  auto import_first_note_octave = [this, &window_body](const QString& transpose,
                                                       int& octave) -> void {
    QTemporaryFile temp_file;
    QVERIFY(temp_file.open());
    temp_file.write(make_musicxml(get_divisions() +
                                      "<transpose><chromatic>0</chromatic>" +
                                      transpose + "</transpose>",
                                  make_pitch_note("", "1"))
                        .toStdString()
                        .c_str());
    temp_file.close();
    import_musicxml_and_reload(
        main_window.song_menu_bar, main_window.window_body,
        main_window.piano_roll_widget, temp_file.fileName());
    octave = window_body.song.chords.at(0).pitched_notes.at(0).interval.octave;
  };

  auto plain_octave = 0;
  import_first_note_octave("", plain_octave);
  auto shifted_octave = 0;
  import_first_note_octave("<octave-change>1</octave-change>", shifted_octave);
  QCOMPARE(shifted_octave, plain_octave + 1);

  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

// a transposing part's key sounds transposed, like its notes: an A clarinet
// written in C plays in A
void Tester::test_musicxml_transposed_key() {
  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write(
      make_musicxml(get_divisions() + make_key("0") +
                        "<transpose><chromatic>-3</chromatic></transpose>",
                    make_pitch_note("", "1"))
          .toStdString()
          .c_str());
  temp_file.close();

  auto& window_body = main_window.window_body;
  import_musicxml_and_reload(main_window.song_menu_bar, main_window.window_body,
                             main_window.piano_roll_widget,
                             temp_file.fileName());
  QCOMPARE(window_body.controls_column.spin_boxes.starting_key_editor.value(),
           midi_number_to_frequency(MIDDLE_C_MIDI + 9));
  // so the written tonic is the tonic
  const auto& interval =
      window_body.song.chords.at(0).pitched_notes.at(0).interval;
  QCOMPARE(interval.ratio.numerator, 1);
  QCOMPARE(interval.ratio.denominator, 1);
  QCOMPARE(interval.octave, -1);

  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

// notes are spelled from their accidentals, as they would be read, with
// arrows as Johnston's 7 (down, 35/36) and el (up, 36/35); the checked note
// is the first in the last chord, measured from the key
void Tester::test_musicxml_accidentals_data() {
  QTest::addColumn<QString>("attributes");
  QTest::addColumn<QString>("body");
  QTest::addColumn<Interval>("expected_interval");

  const auto c_major = get_divisions() + make_key("0");

  QTest::newRow("minor seventh")
      << c_major << make_spelled_note("B", "4", "flat")
      << Interval(Rational(9, 5));
  QTest::newRow("harmonic seventh")
      << c_major << make_spelled_note("B", "4", "flat-down")
      << Interval(Rational(7, 4));
  QTest::newRow("el") << c_major << make_spelled_note("B", "4", "flat-up")
                      << Interval(Rational(324, 175));
  QTest::newRow("natural with 7")
      << c_major << make_spelled_note("B", "4", "natural-down")
      << Interval(Rational(175, 96));
  // not the same as an F raised by an el, even though both are notated a
  // quartertone above F
  QTest::newRow("sharp with 7")
      << c_major << make_spelled_note("F", "4", "sharp-down")
      << Interval(Rational(175, 128));
  QTest::newRow("natural with el")
      << c_major << make_spelled_note("F", "4", "natural-up")
      << Interval(Rational(48, 35));
  QTest::newRow("double flat with 7")
      << c_major << make_spelled_note("B", "4", "flat-flat-down")
      << Interval(Rational(175, 108));
  QTest::newRow("double sharp with el")
      << c_major << make_spelled_note("C", "4", "double-sharp-up")
      << Interval(Rational(81, 70));
  // <alter> is ignored in favor of the accidental
  QTest::newRow("alter ignored") << c_major << R"(
      <note>
        <pitch><step>B</step><alter>-1</alter><octave>4</octave></pitch>
        <duration>1</duration>
      </note>)" << Interval(Rational(15, 8));
  // F major: B is flat, and the key is F
  QTest::newRow("key signature flat")
      << get_divisions() + make_key("-1") << make_spelled_note("B", "4")
      << Interval(Rational(4, 3));
  // G# major: the eighth sharp doubles F, and the key is G#
  QTest::newRow("key signature double sharp")
      << get_divisions() + make_key("8") << make_spelled_note("F", "4")
      << Interval(Rational(15, 8), -1);
  QTest::newRow("accidental lasts through the measure")
      << c_major
      << make_spelled_note("B", "4", "flat-down") + make_spelled_note("B", "4")
      << Interval(Rational(7, 4));
  QTest::newRow("accidental replaced later in the measure")
      << c_major
      << make_spelled_note("B", "4", "flat-down") +
             make_spelled_note("B", "4", "flat")
      << Interval(Rational(9, 5));
  QTest::newRow("accidental only applies to its octave")
      << c_major
      << make_spelled_note("B", "4", "flat-down") + make_spelled_note("B", "3")
      << Interval(Rational(15, 8), -1);
  QTest::newRow("accidental only applies to its staff")
      << c_major
      << make_spelled_note("B", "4", "flat-down", "", "1") +
             make_spelled_note("B", "4", "", "", "2")
      << Interval(Rational(15, 8));
  QTest::newRow("accidental ends with the measure")
      << c_major
      << make_spelled_note("B", "4", "flat-down") + get_next_measure() +
             make_spelled_note("B", "4")
      << Interval(Rational(15, 8));
  // once an accidental stops applying, the note falls back to the key
  // signature, not to natural
  QTest::newRow("key signature after the accidental's measure")
      << get_divisions() + make_key("-1")
      << make_spelled_note("B", "4", "natural") + get_next_measure() +
             make_spelled_note("B", "4")
      << Interval(Rational(4, 3));
  QTest::newRow("key signature outside the accidental's octave")
      << get_divisions() + make_key("-1")
      << make_spelled_note("B", "4", "natural") + make_spelled_note("B", "3")
      << Interval(Rational(4, 3), -1);
  // a tied-over note keeps the pitch it was tied from, without an accidental
  QTest::newRow("tie continues across the barline")
      << c_major
      << make_spelled_note("B", "4", "flat-down", "start") +
             get_next_measure() + make_spelled_note("B", "4", "", "stop")
      << Interval(Rational(7, 4));
}

void Tester::test_musicxml_accidentals() {
  QFETCH(const QString, attributes);
  QFETCH(const QString, body);
  QFETCH(const Interval, expected_interval);

  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write(make_musicxml(attributes, body).toStdString().c_str());
  temp_file.close();

  auto& window_body = main_window.window_body;
  import_musicxml_and_reload(main_window.song_menu_bar, main_window.window_body,
                             main_window.piano_roll_widget,
                             temp_file.fileName());
  const auto& chords = window_body.song.chords;
  QVERIFY(!chords.isEmpty());
  const auto& pitched_notes = chords.last().pitched_notes;
  QVERIFY(!pitched_notes.isEmpty());
  const auto& interval = pitched_notes.at(0).interval;
  QCOMPARE(interval.ratio.numerator, expected_interval.ratio.numerator);
  QCOMPARE(interval.ratio.denominator, expected_interval.ratio.denominator);
  QCOMPARE(interval.octave, expected_interval.octave);

  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

// every imported voice needs a unique, non-empty name, so an unnamed part
// gets a placeholder and a repeated part name gets a numbered suffix
void Tester::test_import_musicxml_voice_names_deduplicated() {
  static const QList<QString> part_names = {"", "Flute", "Flute"};

  QString part_list;
  QString parts;
  for (auto part_number = 1; part_number <= part_names.size();
       part_number = part_number + 1) {
    const auto part_id = QString("P%1").arg(part_number);
    part_list += QString(R"(
    <score-part id="%1"><part-name>%2</part-name></score-part>)")
                     .arg(part_id, part_names.at(part_number - 1));
    parts += QString(R"(
  <part id="%1">
    <measure number="1">
      <attributes>%2</attributes>
      %3
    </measure>
  </part>)")
                 .arg(part_id, get_divisions(), make_pitch_note("", "1"));
  }

  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write(QString(R"(
<score-partwise version="4.0">
  <part-list>%1
  </part-list>%2
</score-partwise>)")
                      .arg(part_list, parts)
                      .toStdString()
                      .c_str());
  temp_file.close();

  import_musicxml_and_reload(main_window.song_menu_bar, main_window.window_body,
                             main_window.piano_roll_widget,
                             temp_file.fileName());

  const auto& pitched_voices = main_window.window_body.song.pitched_voices;
  QCOMPARE(pitched_voices.size(), 3);
  QCOMPARE(pitched_voices.at(0).name, QString("Unnamed instrument"));
  QCOMPARE(pitched_voices.at(1).name, QString("Flute"));
  QCOMPARE(pitched_voices.at(2).name, QString("Flute (2)"));

  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

// a first ending plays only on the first pass and a second ending only on
// the second; a repeated ending number counts once, and a blank ending
// number (allowed by the schema for an ending of unknown type) marks no
// passes, so its measure just plays once after the repeat
void Tester::test_musicxml_endings() {
  // every %1 is the same plain note
  const auto body = QString(R"(
      <barline location="left"><repeat direction="forward"/></barline>
      %1
    </measure>
    <measure number="2">
      <barline location="left"><ending number="1, 1" type="start"/></barline>
      %1
      <barline location="right">
        <ending number="1" type="stop"/>
        <repeat direction="backward"/>
      </barline>
    </measure>
    <measure number="3">
      <barline location="left"><ending number="2" type="start"/></barline>
      %1
      <barline location="right">
        <ending number="2" type="discontinue"/>
      </barline>
    </measure>
    <measure number="4">
      <barline location="left"><ending number=" " type="start"/></barline>
      %1
      <barline location="right"><ending number=" " type="stop"/></barline>)")
                        .arg(make_pitch_note("", "1"));

  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write(make_musicxml(get_divisions(), body).toStdString().c_str());
  temp_file.close();

  import_musicxml_and_reload(main_window.song_menu_bar, main_window.window_body,
                             main_window.piano_roll_widget,
                             temp_file.fileName());
  // measures 1, 2 (first ending), 1, 3 (second ending), 4
  QCOMPARE(main_window.window_body.song.chords.size(), 5);

  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

namespace {

// one quarter note per measure, each followed by that measure's extras
auto make_measures(const QList<QString>& measure_extras) -> QString {
  QString body;
  for (auto index = 0; index < measure_extras.size(); index = index + 1) {
    if (index > 0) {
      body += get_next_measure();
    }
    body += make_pitch_note("", "1") + measure_extras.at(index);
  }
  return body;
}

auto get_forward_repeat() -> QString {
  return R"(<barline location="left"><repeat direction="forward"/></barline>)";
}

auto get_backward_repeat() -> QString {
  return R"(<barline><repeat direction="backward"/></barline>)";
}

}  // namespace

// jumps are taken at the end of their measure. After a da capo or dal segno,
// repeats are skipped, only the last ending is played, and a fine or to coda
// applies
void Tester::test_musicxml_jumps_data() {
  QTest::addColumn<QString>("body");
  QTest::addColumn<QList<int>>("expected_measures");

  const QString da_capo = R"(<sound dacapo="yes"/>)";
  const QString dal_segno = R"(<sound dalsegno="segno"/>)";
  const QString segno = R"(<sound segno="segno"/>)";
  const QString to_coda = R"(<sound tocoda="coda"/>)";
  const QString coda = R"(<sound coda="coda"/>)";
  const QString fine = R"(<sound fine="yes"/>)";

  QTest::newRow("da capo")
      << make_measures({"", da_capo}) << QList<int>({1, 2, 1, 2});
  QTest::newRow("da capo in a direction")
      << make_measures(
             {"", R"(<direction>
        <direction-type><words>D.C.</words></direction-type>
        <sound dacapo="yes"/>
      </direction>)"})
      << QList<int>({1, 2, 1, 2});
  QTest::newRow("da capo al fine")
      << make_measures({fine, "", da_capo}) << QList<int>({1, 2, 3, 1});
  QTest::newRow("dal segno")
      << make_measures({"", segno, dal_segno})
      << QList<int>({1, 2, 3, 2, 3});
  QTest::newRow("segno on a barline")
      << make_measures(
             {"", R"(<barline location="left" segno="segno"><segno/></barline>)",
              dal_segno})
      << QList<int>({1, 2, 3, 2, 3});
  // with no segno by that name, go back to the nearest one
  QTest::newRow("unnamed segno")
      << make_measures({"", R"(<sound segno=""/>)", dal_segno})
      << QList<int>({1, 2, 3, 2, 3});
  QTest::newRow("dal segno al coda")
      << make_measures({segno, to_coda, dal_segno, coda})
      << QList<int>({1, 2, 3, 1, 2, 4});
  QTest::newRow("repeats skipped after a da capo")
      << make_measures({get_forward_repeat(), get_backward_repeat(), da_capo})
      << QList<int>({1, 2, 1, 2, 3, 1, 2, 3});
  QTest::newRow("last ending after a da capo")
      << make_measures(
             {get_forward_repeat(),
              R"(<barline location="left"><ending number="1" type="start"/></barline>
      <barline>
        <ending number="1" type="stop"/>
        <repeat direction="backward"/>
      </barline>)",
              R"(<barline location="left"><ending number="2" type="start"/></barline>
      <barline><ending number="2" type="discontinue"/></barline>)" +
                  fine,
              da_capo})
      << QList<int>({1, 2, 1, 3, 4, 1, 3});
  QTest::newRow("time-only")
      << make_measures({R"(<sound tocoda="coda" time-only="1"/>)", "", coda})
      << QList<int>({1, 3});
  QTest::newRow("hidden forward repeat")
      << make_measures(
             {"", R"(<sound forward-repeat="yes"/>)", get_backward_repeat()})
      << QList<int>({1, 2, 3, 2, 3});
}

void Tester::test_musicxml_jumps() {
  QFETCH(const QString, body);
  QFETCH(const QList<int>, expected_measures);

  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write(make_musicxml(get_divisions(), body).toStdString().c_str());
  temp_file.close();

  import_musicxml_and_reload(main_window.song_menu_bar, main_window.window_body,
                             main_window.piano_roll_widget,
                             temp_file.fileName());
  // each chord's words are its measure number
  QList<int> measures;
  for (const auto& chord : main_window.window_body.song.chords) {
    measures.push_back(chord.words.toInt());
  }
  QCOMPARE(measures, expected_measures);

  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

// jumps are often only written in the top part, but every part follows them
void Tester::test_musicxml_jump_in_one_part() {
  const auto make_part = [](const QString& part_id,
                            const QString& last_measure_extras) -> QString {
    return QString(R"(
  <part id="%1">
    <measure number="1">
      <attributes>%2</attributes>
      %3)")
        .arg(part_id, get_divisions(),
             make_measures({"", last_measure_extras})) +
           R"(
    </measure>
  </part>)";
  };

  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write((QString(R"(
<score-partwise version="4.0">
  <part-list>
    <score-part id="P1"><part-name>P1</part-name></score-part>
    <score-part id="P2"><part-name>P2</part-name></score-part>
  </part-list>)") + make_part("P1", R"(<sound dacapo="yes"/>)") +
                   make_part("P2", "") + R"(
</score-partwise>)")
                      .toStdString()
                      .c_str());
  temp_file.close();

  import_musicxml_and_reload(main_window.song_menu_bar, main_window.window_body,
                             main_window.piano_roll_widget,
                             temp_file.fileName());
  const auto& chords = main_window.window_body.song.chords;
  QCOMPARE(chords.size(), 4);
  for (const auto& chord : chords) {
    QCOMPARE(chord.pitched_notes.size(), 2);
  }

  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}
