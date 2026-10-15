#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLabel>

#include "Tester.hpp"
#include "widgets/ControlsColumn.hpp"
#include "widgets/SpinBoxes.hpp"

namespace {

auto get_not_musicxml_message() -> QString {
  return "music21 can't read the score: Cannot parse MusicXML files not in "
         "score-partwise. Root tag was 'song'";
}

}  // namespace

void Tester::test_musicxml_data() {
  QTest::addColumn<QString>("file_name");
  QTest::addColumn<int>("number_of_chords");

  QTest::newRow("prelude") << "prelude.musicxml" << MUSIC_XML_ROWS;
  QTest::newRow("compressed prelude") << "prelude.mxl" << MUSIC_XML_ROWS;
  QTest::newRow("percussion") << "percussion.musicxml" << PERCUSSION_ROWS;
  QTest::newRow("repeats") << "Saltarello.musicxml" << SALTARELLO_ROWS;
  // grace notes have no duration, so they're left out
  QTest::newRow("grace notes") << "MozartPianoSonata.musicxml" << 24;
  QTest::newRow("fractional divisions") << "fractional_divisions.musicxml"
                                        << 1;
  // music21 doesn't require a tie-stop to have a matching tie-start
  QTest::newRow("orphan tie") << "orphan_tie.musicxml" << 1;
}

void Tester::test_musicxml() {
  QFETCH(const QString, file_name);
  QFETCH(const int, number_of_chords);

  auto& window_body = main_window.window_body;

  import_musicxml(main_window, test_dir.filePath(file_name));
  QCOMPARE(window_body.song.chords.size(), number_of_chords);

  // an imported score has to fit song.xsd, or it would save but not reopen
  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.close();
  save_as_file(window_body, temp_file.fileName());
  QVERIFY(open_file(main_window, temp_file.fileName()));
  QCOMPARE(window_body.song.chords.size(), number_of_chords);

  open_file(main_window, test_dir.filePath("test_song.justly"));
}

void Tester::test_musicxml_error_data() {
  QTest::addColumn<QString>("file_name");
  QTest::addColumn<QString>("error_message");

  QTest::newRow("not musicxml")
      << "not_musicxml.xml" << get_not_musicxml_message();
  QTest::newRow("invalid mxl")
      << "invalid.mxl"
      << "music21 can't read the score: syntax error: line 1, column 0";
  // a compressed score's container.xml has to exist, parse, and name a
  // rootfile
  QTest::newRow("mxl without container")
      << "mxl_no_container.mxl"
      << "music21 can't read the score: Cannot parse MusicXML files not in "
         "score-partwise. Root tag was 'x'";
  const QString missing_rootfile_message =
      "music21 can't read the score: a bytes-like object is required, not "
      "'NoneType'";
  QTest::newRow("mxl with invalid container")
      << "mxl_bad_container.mxl" << missing_rootfile_message;
  QTest::newRow("mxl container without rootfiles")
      << "mxl_no_rootfiles.mxl" << missing_rootfile_message;
  QTest::newRow("mxl container without rootfile")
      << "mxl_no_rootfile.mxl" << missing_rootfile_message;
  QTest::newRow("empty") << "empty.musicxml" << "No chords";
  // music21 transposes the A clarinet's key signature as if it were a Bb
  // clarinet, the instrument its midi program names, while it transposes
  // its notes by its transpose element
  QTest::newRow("transposing instrument")
      << "MozartTrio.musicxml"
      << "The key signatures in measure 0 don't match";
  QTest::newRow("timewise")
      << "timewise.musicxml"
      << "music21 can't read the score: Cannot parse MusicXML files not in "
         "score-partwise. Root tag was 'score-timewise'";
}

void Tester::test_musicxml_error() {
  QFETCH(const QString, error_message);
  QFETCH(const QString, file_name);

  close_message_later(main_window, waiting_for_message, error_message);
  import_musicxml(main_window, test_dir.filePath(file_name));
}

// divisions is unbounded xs:decimal with no schema-enforced range. Python
// 3.14 dropped the "float" from the error message, and on Linux, the
// system's python could be either
void Tester::test_musicxml_overflowing_divisions() {
  check_message_later(
      main_window, waiting_for_message,
      [](const QString& actual_text) -> auto {
        QVERIFY(QStringList(
                    {"music21 can't read the score: float division by zero",
                     "music21 can't read the score: division by zero"})
                    .contains(actual_text));
      });
  import_musicxml(main_window,
                  test_dir.filePath("overflowing_divisions.musicxml"));
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

  import_musicxml(main_window, test_dir.filePath("prelude.musicxml"));
  QCOMPARE(get_model(switch_table).rowCount(QModelIndex()), MUSIC_XML_ROWS);

  open_file(main_window, test_dir.filePath("test_song.justly"));
}

// regression test: opening/importing a file bypasses the undo stack, so
// replace_table (which normally resets the switch column's label and the
// view menu's actions whenever the table switches to a different row
// type) never runs for them; open_file/import_musicxml must reapply that
// same reset explicitly (via finish_loading) instead of
// leaving the label/actions stuck showing whatever was being edited before
void Tester::test_open_after_editing_chord_notes_resets_menu() {
  auto& window_body = main_window.window_body;
  auto& switch_column = window_body.switch_column;
  auto& view_menu = main_window.song_menu_bar.view_menu;

  switch_to(main_window, RowType::pitched_note_type, 0);
  QCOMPARE(switch_column.editing_text.text(), "Pitched notes for chord 1");
  QVERIFY(view_menu.back_to_chords_action.isEnabled());

  open_file(main_window, test_dir.filePath("test_song.justly"));

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
  auto& open_action = main_window.song_menu_bar.file_menu.open_action;

  QVERIFY(open_action.isEnabled());

  switch_to(main_window, RowType::pitched_voice_type, -1);
  QVERIFY(open_action.isEnabled());
  maybe_switch_back_to_chords(main_window, RowType::pitched_voice_type);

  switch_to(main_window, RowType::pitched_note_type, 0);
  QVERIFY(open_action.isEnabled());
  maybe_switch_back_to_chords(main_window, RowType::pitched_note_type);
}

// regression test: finish_loading (which resets the switch column's label/
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
                      get_not_musicxml_message());
  import_musicxml(main_window, test_dir.filePath("not_musicxml.xml"));

  QCOMPARE(switch_column.editing_text.text(), "Pitched notes for chord 1");

  maybe_switch_back_to_chords(main_window, RowType::pitched_note_type);
}

// an imported voice whose name exactly matches a built-in program (here, the
// part is named "Marimba") should use that program instead of the default
void Tester::test_import_musicxml_voice_named_like_program() {
  auto& window_body = main_window.window_body;

  import_musicxml(main_window,
                  test_dir.filePath("program_named_part.musicxml"));

  const auto& pitched_voices = window_body.song.pitched_voices;
  QCOMPARE(pitched_voices.size(), 1);
  QCOMPARE(pitched_voices.at(0).name, QString("Marimba"));
  QCOMPARE(pitched_voices.at(0).program, QString("Marimba"));

  open_file(main_window, test_dir.filePath("test_song.justly"));
}

// a part's midi-program picks its voice's program, here for a voice named
// "Violin II", which doesn't match a program by name
void Tester::test_import_musicxml_midi_program() {
  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write(R"(
<score-partwise version="4.0">
  <part-list>
    <score-part id="P1">
      <part-name>Second violin</part-name>
      <score-instrument id="P1-I1">
        <instrument-name>Violin II</instrument-name>
      </score-instrument>
      <midi-instrument id="P1-I1"><midi-program>41</midi-program></midi-instrument>
    </score-part>
  </part-list>
  <part id="P1">
    <measure number="1">
      <attributes><divisions>1</divisions></attributes>
      <note><pitch><step>C</step><octave>4</octave></pitch><duration>1</duration></note>
    </measure>
  </part>
</score-partwise>)");
  temp_file.close();

  auto& window_body = main_window.window_body;
  import_musicxml(main_window, temp_file.fileName());

  const auto& pitched_voices = window_body.song.pitched_voices;
  QCOMPARE(pitched_voices.size(), 1);
  QCOMPARE(pitched_voices.at(0).name, QString("Violin II"));
  QCOMPARE(pitched_voices.at(0).program, QString("Violin"));

  open_file(main_window, test_dir.filePath("test_song.justly"));
}

// a metronome mark changes the tempo, rounded to a whole number of quarter
// notes per minute. Repeating back to before a change goes back to the
// earlier tempo. music21 reads a metronome mark over the sound's tempo next
// to it, and skips a metronome mark it can't read a number from, like
// "c. 40"
void Tester::test_import_musicxml_tempo() {
  auto& window_body = main_window.window_body;

  import_musicxml(main_window, test_dir.filePath("tempo.musicxml"));

  const auto& song = window_body.song;
  QCOMPARE(song.starting_tempo, 60);

  // C, D, then repeated, then E, F
  static const QList<Rational> expected_tempo_ratios = {
      Rational(1), Rational(1), Rational(1),
      Rational(1), Rational(1), Rational(5, 2)};
  QCOMPARE(song.chords.size(), expected_tempo_ratios.size());
  for (auto chord_number = 0; chord_number < song.chords.size();
       ++chord_number) {
    QCOMPARE(song.chords.at(chord_number).tempo_ratio,
             expected_tempo_ratios.at(chord_number));
  }

  open_file(main_window, test_dir.filePath("test_song.justly"));
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

// a quarter note (with one division per quarter), optionally altered, tied,
// with an accidental, or on a given staff
auto make_spelled_note(const QString& step, const QString& octave,
                       const QString& alter = "",
                       const QString& accidental = "",
                       const QString& tie_type = "", const QString& staff = "")
    -> QString {
  return "<note><pitch><step>" + step + "</step>" +
         (alter.isEmpty() ? QString() : "<alter>" + alter + "</alter>") +
         "<octave>" + octave + "</octave></pitch><duration>1</duration>" +
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

// a direction with the given words, which is how music21 reads jumps
auto get_words(const QString& words) -> QString {
  return "<direction><direction-type><words>" + words +
         "</words></direction-type></direction>";
}

}  // namespace

// scores that can't be imported
void Tester::test_musicxml_inline_error_data() {
  QTest::addColumn<QString>("attributes");
  QTest::addColumn<QString>("body");
  QTest::addColumn<QString>("error_message");

  const auto plain_note = make_pitch_note("", "1");
  const auto unfollowable_message =
      QString("The score's repeats, endings, or jumps can't be followed");

  QTest::newRow("microtonal transpose")
      << get_divisions() + "<transpose><chromatic>1.5</chromatic></transpose>"
      << plain_note
      << "music21 can't read the score: invalid literal for int() with base "
         "10: '1.5'";
  QTest::newRow("note outside the midi range")
      << get_divisions() + "<transpose><chromatic>" + get_huge_number() +
             "</chromatic></transpose>"
      << plain_note << "A note in measure 1 is outside the midi range";
  // only arrows are read as septimal quartertones, since a bare quartertone
  // accidental doesn't say which chromatic accidental it modifies
  QTest::newRow("quartertone accidental")
      << get_divisions() << make_pitch_note("quarter-flat", "1")
      << "Accidental half-flat in measure 1 is not supported";
  QTest::newRow("note duration out of range")
      << get_divisions() << make_pitch_note("", get_huge_number())
      << "The score is too long, or its rhythms too fine";
  // beats are ratios with at most 999 in the numerator and denominator
  QTest::newRow("note too long for a ratio")
      << get_divisions() << make_pitch_note("", "1000")
      << "A rhythm in measure 1 is too long or too fine to write as a ratio";
  QTest::newRow("note too fine for a ratio")
      << "<divisions>1000</divisions>" << make_pitch_note("", "1")
      << "A rhythm in measure 1 is too long or too fine to write as a ratio";
  QTest::newRow("tempo too fast")
      << get_divisions()
      << R"(<direction><sound tempo="1000"/></direction>)" + plain_note
      << "The tempo in measure 1 is faster than 999 bpm";
  // unrolling a repeat copies its measures, so too many would run out of
  // memory
  QTest::newRow("too many repeat times")
      << get_divisions()
      << plain_note +
             QString(
                 R"(<barline><repeat direction="backward" times="%1"/></barline>)")
                 .arg(get_huge_number())
      << "The repeat in measure 1 plays more than 100 times";
  QTest::newRow("forward repeat at the end")
      << get_divisions()
      << R"(<barline location="left"><repeat direction="forward"/></barline>)" +
             plain_note
      << unfollowable_message;
  QTest::newRow("forward repeat before another")
      << get_divisions()
      << R"(<barline location="left"><repeat direction="forward"/></barline>)" +
             plain_note + get_next_measure() +
             R"(<barline location="left"><repeat direction="forward"/></barline>)" +
             plain_note + R"(<barline><repeat direction="backward"/></barline>)"
      << unfollowable_message;
  // Justly has one key at a time
  QTest::newRow("staves in different keys")
      << get_divisions() + "<staves>2</staves>" +
             R"(<key number="1"><fifths>1</fifths></key>)" +
             R"(<key number="2"><fifths>0</fifths></key>)"
      << make_spelled_note("B", "4", "", "", "", "1") +
             make_spelled_note("C", "3", "", "", "", "2")
      << "The key signatures in measure 1 don't match";
  QTest::newRow("dal segno without a segno")
      << get_divisions() << plain_note + get_words("D.S.")
      << unfollowable_message;
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
  import_musicxml(main_window, temp_file.fileName());
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
  import_musicxml(main_window, temp_file.fileName());
  QCOMPARE(window_body.song.chords.size(), 3);

  open_file(main_window, test_dir.filePath("test_song.justly"));
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
                                      "<transpose><diatonic>0</diatonic>"
                                      "<chromatic>0</chromatic>" +
                                      transpose + "</transpose>",
                                  make_pitch_note("", "1"))
                        .toStdString()
                        .c_str());
    temp_file.close();
    import_musicxml(main_window, temp_file.fileName());
    octave = window_body.song.chords.at(0).pitched_notes.at(0).interval.octave;
  };

  auto plain_octave = 0;
  import_first_note_octave("", plain_octave);
  auto shifted_octave = 0;
  import_first_note_octave("<octave-change>1</octave-change>", shifted_octave);
  QCOMPARE(shifted_octave, plain_octave + 1);

  open_file(main_window, test_dir.filePath("test_song.justly"));
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
  import_musicxml(main_window, temp_file.fileName());
  QCOMPARE(window_body.controls_column.spin_boxes.starting_key_editor.value(),
           midi_number_to_frequency(MIDDLE_C_MIDI + 9));
  // so the written tonic is the tonic
  const auto& interval =
      window_body.song.chords.at(0).pitched_notes.at(0).interval;
  QCOMPARE(interval.ratio.numerator, 1);
  QCOMPARE(interval.ratio.denominator, 1);
  QCOMPARE(interval.octave, -1);

  open_file(main_window, test_dir.filePath("test_song.justly"));
}

// music21 reads each note's pitch from its alter, and Justly reads arrows on
// its accidental as Johnston's 7 (down, 35/36) and el (up, 36/35); the
// checked note is the first in the last chord, measured from the key
void Tester::test_musicxml_accidentals_data() {
  QTest::addColumn<QString>("attributes");
  QTest::addColumn<QString>("body");
  QTest::addColumn<Interval>("expected_interval");

  const auto c_major = get_divisions() + make_key("0");

  QTest::newRow("minor seventh")
      << c_major << make_spelled_note("B", "4", "-1", "flat")
      << Interval(Rational(9, 5));
  QTest::newRow("harmonic seventh")
      << c_major << make_spelled_note("B", "4", "-1", "flat-down")
      << Interval(Rational(7, 4));
  QTest::newRow("el") << c_major
                      << make_spelled_note("B", "4", "-1", "flat-up")
                      << Interval(Rational(324, 175));
  QTest::newRow("natural with 7")
      << c_major << make_spelled_note("B", "4", "", "natural-down")
      << Interval(Rational(175, 96));
  // not the same as an F raised by an el, even though both are notated a
  // quartertone above F
  QTest::newRow("sharp with 7")
      << c_major << make_spelled_note("F", "4", "1", "sharp-down")
      << Interval(Rational(175, 128));
  QTest::newRow("natural with el")
      << c_major << make_spelled_note("F", "4", "", "natural-up")
      << Interval(Rational(48, 35));
  QTest::newRow("double flat with 7")
      << c_major << make_spelled_note("B", "4", "-2", "flat-flat-down")
      << Interval(Rational(175, 108));
  QTest::newRow("double sharp with el")
      << c_major << make_spelled_note("C", "4", "2", "double-sharp-up")
      << Interval(Rational(81, 70));
  QTest::newRow("alter without an accidental")
      << c_major << make_spelled_note("B", "4", "-1")
      << Interval(Rational(9, 5));
  // F major: the key is F
  QTest::newRow("key signature flat")
      << get_divisions() + make_key("-1") << make_spelled_note("B", "4", "-1")
      << Interval(Rational(4, 3));
  // G# major: the key is G#
  QTest::newRow("key signature double sharp")
      << get_divisions() + make_key("8") << make_spelled_note("F", "4", "2")
      << Interval(Rational(15, 8), -1);
  QTest::newRow("arrow lasts through the measure")
      << c_major
      << make_spelled_note("B", "4", "-1", "flat-down") +
             make_spelled_note("B", "4", "-1")
      << Interval(Rational(7, 4));
  QTest::newRow("arrow replaced later in the measure")
      << c_major
      << make_spelled_note("B", "4", "-1", "flat-down") +
             make_spelled_note("B", "4", "-1", "flat")
      << Interval(Rational(9, 5));
  QTest::newRow("arrow only applies to its octave")
      << c_major
      << make_spelled_note("B", "4", "-1", "flat-down") +
             make_spelled_note("B", "3")
      << Interval(Rational(15, 8), -1);
  QTest::newRow("arrow only applies to its staff")
      << c_major + "<staves>2</staves>"
      << make_spelled_note("B", "4", "-1", "flat-down", "", "1") +
             make_spelled_note("B", "4", "", "", "", "2")
      << Interval(Rational(15, 8));
  QTest::newRow("arrow ends with the measure")
      << c_major
      << make_spelled_note("B", "4", "-1", "flat-down") + get_next_measure() +
             make_spelled_note("B", "4")
      << Interval(Rational(15, 8));
  // a tied-over note keeps the arrow it was tied from
  QTest::newRow("tie continues across the barline")
      << c_major
      << make_spelled_note("B", "4", "-1", "flat-down", "start") +
             get_next_measure() +
             make_spelled_note("B", "4", "-1", "", "stop")
      << Interval(Rational(7, 4));
  // an alter can't hold a 7 or el, so files leave it out or get it wrong,
  // and an arrow's accidental spells the note instead
  QTest::newRow("arrow without an alter")
      << c_major << make_spelled_note("B", "4", "", "flat-down")
      << Interval(Rational(7, 4));
  QTest::newRow("arrow with a wrong alter")
      << c_major << make_spelled_note("B", "4", "-1.2", "flat-down")
      << Interval(Rational(7, 4));
  QTest::newRow("arrow without an alter lasts through the measure")
      << c_major
      << make_spelled_note("B", "4", "", "flat-down") +
             make_spelled_note("B", "4")
      << Interval(Rational(7, 4));
  QTest::newRow("arrow without an alter continues across a tie")
      << c_major
      << make_spelled_note("B", "4", "", "flat-down", "start") +
             get_next_measure() + make_spelled_note("B", "4", "", "", "stop")
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
  import_musicxml(main_window, temp_file.fileName());
  const auto& chords = window_body.song.chords;
  QVERIFY(!chords.isEmpty());
  const auto& pitched_notes = chords.last().pitched_notes;
  QVERIFY(!pitched_notes.isEmpty());
  const auto& interval = pitched_notes.at(0).interval;
  QCOMPARE(interval.ratio.numerator, expected_interval.ratio.numerator);
  QCOMPARE(interval.ratio.denominator, expected_interval.ratio.denominator);
  QCOMPARE(interval.octave, expected_interval.octave);

  open_file(main_window, test_dir.filePath("test_song.justly"));
}

namespace {

// a score with a one-note part for each name
auto make_named_parts(const QList<QString>& part_names) -> QString {
  QString part_list;
  QString parts;
  for (auto part_number = 1; part_number <= part_names.size(); ++part_number) {
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
  return QString(R"(
<score-partwise version="4.0">
  <part-list>%1
  </part-list>%2
</score-partwise>)")
      .arg(part_list, parts);
}

}  // namespace

// every imported voice needs a non-empty name, so an unnamed part gets a
// placeholder
void Tester::test_import_musicxml_unnamed_voice() {
  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write(make_named_parts({"", "Flute"}).toStdString().c_str());
  temp_file.close();

  import_musicxml(main_window, temp_file.fileName());

  const auto& pitched_voices = main_window.window_body.song.pitched_voices;
  QCOMPARE(pitched_voices.size(), 2);
  QCOMPARE(pitched_voices.at(0).name, QString("Unnamed instrument"));
  QCOMPARE(pitched_voices.at(1).name, QString("Flute"));

  open_file(main_window, test_dir.filePath("test_song.justly"));
}

// voices are looked up by name, so a repeated part name, or a second unnamed
// part, gets a numbered suffix
void Tester::test_import_musicxml_duplicate_voice_names() {
  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write(
      make_named_parts({"Flute", "", "Flute", ""}).toStdString().c_str());
  temp_file.close();

  import_musicxml(main_window, temp_file.fileName());

  const auto& pitched_voices = main_window.window_body.song.pitched_voices;
  QCOMPARE(pitched_voices.size(), 4);
  QCOMPARE(pitched_voices.at(0).name, QString("Flute"));
  QCOMPARE(pitched_voices.at(1).name, QString("Unnamed instrument"));
  QCOMPARE(pitched_voices.at(2).name, QString("Flute (2)"));
  QCOMPARE(pitched_voices.at(3).name, QString("Unnamed instrument (2)"));

  open_file(main_window, test_dir.filePath("test_song.justly"));
}

// a first ending plays only on the first pass and a second ending only on
// the second
void Tester::test_musicxml_endings() {
  // every %1 is the same plain note
  const auto body = QString(R"(
      <barline location="left"><repeat direction="forward"/></barline>
      %1
    </measure>
    <measure number="2">
      <barline location="left"><ending number="1" type="start"/></barline>
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
      </barline>)")
                        .arg(make_pitch_note("", "1"));

  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write(make_musicxml(get_divisions(), body).toStdString().c_str());
  temp_file.close();

  import_musicxml(main_window, temp_file.fileName());
  // measures 1, 2 (first ending), 1, 3 (second ending)
  QCOMPARE(main_window.window_body.song.chords.size(), 4);

  open_file(main_window, test_dir.filePath("test_song.justly"));
}

namespace {

// one quarter note per measure, each followed by that measure's extras
auto make_measures(const QList<QString>& measure_extras) -> QString {
  QString body;
  for (auto index = 0; index < measure_extras.size(); ++index) {
    if (index > 0) {
      body += QString(R"(
    </measure>
    <measure number="%1">)")
                  .arg(index + 1);
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

// music21 reads jumps, and the segnos, codas, and fines they go to, from
// their words and symbols, and takes them at the end of their measure.
// Chords keep their written measure numbers through jumps
void Tester::test_musicxml_jumps_data() {
  QTest::addColumn<QString>("body");
  QTest::addColumn<QList<int>>("expected_measures");

  const auto segno =
      QString("<direction><direction-type><segno/></direction-type></direction>");
  const auto coda =
      QString("<direction><direction-type><coda/></direction-type></direction>");

  QTest::newRow("da capo") << make_measures({"", get_words("D.C.")})
                           << QList<int>({1, 2, 1, 2});
  QTest::newRow("da capo al fine")
      << make_measures({get_words("Fine"), "", get_words("D.C. al Fine")})
      << QList<int>({1, 2, 3, 1});
  QTest::newRow("dal segno")
      << make_measures({"", segno, get_words("D.S.")})
      << QList<int>({1, 2, 3, 2, 3});
  QTest::newRow("dal segno al coda")
      << make_measures(
             {segno, get_words("To Coda"), get_words("D.S. al Coda"), coda})
      << QList<int>({1, 2, 3, 1, 2, 4});
  QTest::newRow("repeats after a da capo")
      << make_measures(
             {get_forward_repeat(), get_backward_repeat(), get_words("D.C.")})
      << QList<int>({1, 2, 1, 2, 3, 1, 2, 3});
}

void Tester::test_musicxml_jumps() {
  QFETCH(const QString, body);
  QFETCH(const QList<int>, expected_measures);

  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write(make_musicxml(get_divisions(), body).toStdString().c_str());
  temp_file.close();

  import_musicxml(main_window, temp_file.fileName());
  // each chord's words are its measure number
  QList<int> measures;
  for (const auto& chord : main_window.window_body.song.chords) {
    measures.push_back(chord.words.toInt());
  }
  QCOMPARE(measures, expected_measures);

  open_file(main_window, test_dir.filePath("test_song.justly"));
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
  </part-list>)") + make_part("P1", get_words("D.C.")) +
                   make_part("P2", "") + R"(
</score-partwise>)")
                      .toStdString()
                      .c_str());
  temp_file.close();

  import_musicxml(main_window, temp_file.fileName());
  const auto& chords = main_window.window_body.song.chords;
  QCOMPARE(chords.size(), 4);
  for (const auto& chord : chords) {
    QCOMPARE(chord.pitched_notes.size(), 2);
  }

  open_file(main_window, test_dir.filePath("test_song.justly"));
}

// ties are followed in the order the measures are played, so a note tied
// into both endings is held into each of them
void Tester::test_musicxml_tie_into_endings() {
  const auto body =
      get_forward_repeat() + make_spelled_note("C", "4", "", "", "start") +
      get_next_measure() +
      R"(<barline location="left"><ending number="1" type="start"/></barline>)" +
      make_spelled_note("C", "4", "", "", "stop") + R"(
      <barline>
        <ending number="1" type="stop"/>
        <repeat direction="backward"/>
      </barline>)" +
      R"(
    </measure>
    <measure number="3">)" +
      R"(<barline location="left"><ending number="2" type="start"/></barline>)" +
      make_spelled_note("C", "4", "", "", "stop") +
      R"(<barline><ending number="2" type="discontinue"/></barline>)";

  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write(make_musicxml(get_divisions(), body).toStdString().c_str());
  temp_file.close();

  import_musicxml(main_window, temp_file.fileName());
  // measure 1, held through the first ending, then again through the second
  const auto& chords = main_window.window_body.song.chords;
  QCOMPARE(chords.size(), 2);
  for (const auto& chord : chords) {
    QCOMPARE(chord.words, QString("1"));
    QCOMPARE(chord.pitched_notes.size(), 1);
    QCOMPARE(chord.pitched_notes.at(0).beats.numerator, 2);
    QCOMPARE(chord.pitched_notes.at(0).beats.denominator, 1);
  }

  open_file(main_window, test_dir.filePath("test_song.justly"));
}

// regression test: music21 can combine the wrong notes when a measure has
// more than one voice, so here it used to leave the low C's second half out,
// and play the end of the tied B again
void Tester::test_musicxml_ties_in_voices() {
  const auto make_note = [](const QString& step, const QString& duration,
                            const QString& tie_types,
                            const QString& voice) -> QString {
    QString ties;
    for (const auto& tie_type : tie_types.split(" ", Qt::SkipEmptyParts)) {
      ties += QString(R"(<tie type="%1"/>)").arg(tie_type);
    }
    return "<note><pitch><step>" + step +
           "</step><octave>3</octave></pitch><duration>" + duration +
           "</duration>" + ties + "<voice>" + voice + "</voice></note>";
  };
  // a whole C, written as tied halves, against a rest, then a B tied through
  // the rest of the measure
  const auto body =
      make_note("C", "8", "start", "1") + make_note("C", "8", "stop", "1") +
      "<backup><duration>16</duration></backup>"
      "<note><rest/><duration>1</duration><voice>2</voice></note>" +
      make_note("B", "3", "start", "2") +
      make_note("B", "4", "stop start", "2") +
      make_note("B", "8", "stop", "2");

  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write(
      make_musicxml("<divisions>4</divisions>", body).toStdString().c_str());
  temp_file.close();

  import_musicxml(main_window, temp_file.fileName());
  const auto& chords = main_window.window_body.song.chords;
  QCOMPARE(chords.size(), 2);
  const auto& c_notes = chords.at(0).pitched_notes;
  QCOMPARE(c_notes.size(), 1);
  QCOMPARE(c_notes.at(0).beats, Rational(4));
  const auto& b_notes = chords.at(1).pitched_notes;
  QCOMPARE(b_notes.size(), 1);
  QCOMPARE(b_notes.at(0).beats, Rational(15, 4));

  open_file(main_window, test_dir.filePath("test_song.justly"));
}
