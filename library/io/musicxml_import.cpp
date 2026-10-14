#include "io/musicxml_import.hpp"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMap>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QSet>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <limits>
#include <numeric>
#include <set>

#include "common/helpers.hpp"
#include "song/Song.hpp"

namespace {

struct ParsedNote {
  int duration = 0;
  int voice_number = 0;
  // for pitched notes, at sounding pitch
  int midi_number = 0;
  // Johnston septimal quartertones (36/35): -1 for a 7, +1 for an el
  int septimal_quartertones = 0;
  QString words;
};

struct ParsedChord {
  QList<ParsedNote> pitched_notes;
  QList<ParsedNote> unpitched_notes;
};

// every part's notes at sounding pitch, with repeats unrolled and ties
// combined, all timed in song_divisions per beat
struct ParsedScore {
  int song_divisions = 1;
  // notes that start at the same time, in any part, form a chord
  QMap<int, ParsedChord> chords;
  // notes name their voices by number from voices
  MusicXMLVoices voices;
  // each keyed by the time of the change
  QMap<int, int> midi_keys;
  QMap<int, int> measure_numbers;
  // in quarter notes per minute
  QMap<int, int> tempos;
};

// long enough for a big score on a slow computer
static const auto CONVERTER_TIMEOUT_MILLISECONDS = 5 * 60 * 1000;

// the python bundled with Justly, which has music21, unless JUSTLY_PYTHON
// names another
auto get_python_program() -> QString {
  const auto environment = QProcessEnvironment::systemEnvironment();
  if (environment.contains("JUSTLY_PYTHON")) {
    return environment.value("JUSTLY_PYTHON");
  }
#ifdef Q_OS_WIN
  return QString::fromStdString(get_share_file("python/python.exe"));
#else
  return QString::fromStdString(get_share_file("python/bin/python3"));
#endif
}

// shows a wait cursor until it goes out of scope
struct WaitCursor {
  WaitCursor() { QApplication::setOverrideCursor(Qt::WaitCursor); }
  ~WaitCursor() { QApplication::restoreOverrideCursor(); }
  WaitCursor(const WaitCursor&) = delete;
  auto operator=(const WaitCursor&) -> WaitCursor& = delete;
  WaitCursor(WaitCursor&&) = delete;
  auto operator=(WaitCursor&&) -> WaitCursor& = delete;
};

// warns and returns nullopt if the converter doesn't run, or can't convert
// the score
auto run_converter(QWidget& parent, const QString& filename)
    -> std::optional<QJsonObject> {
  const auto python_program = get_python_program();
  QProcess process;
  {
    const WaitCursor wait_cursor;
    process.start(
        python_program,
        {QString::fromStdString(get_share_file("import_score.py")), filename});
    if (!process.waitForStarted()) {
      QMessageBox::warning(&parent, QObject::tr("Python error"),
                           QObject::tr("Couldn't run %1").arg(python_program));
      return std::nullopt;
    }
    if (!process.waitForFinished(CONVERTER_TIMEOUT_MILLISECONDS)) {
      process.kill();
      process.waitForFinished();
      QMessageBox::warning(&parent, QObject::tr("Python error"),
                           QObject::tr("Reading the score took too long"));
      return std::nullopt;
    }
  }
  if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
    // e.g. music21 isn't installed
    QMessageBox::warning(
        &parent, QObject::tr("Python error"),
        QObject::tr("Couldn't read the score with music21:\n%1")
            .arg(QString::fromUtf8(process.readAllStandardError()).trimmed()));
    return std::nullopt;
  }
  const auto document =
      QJsonDocument::fromJson(process.readAllStandardOutput());
  if (!document.isObject()) {
    QMessageBox::warning(&parent, QObject::tr("Python error"),
                         QObject::tr("Couldn't understand music21's output"));
    return std::nullopt;
  }
  auto result = document.object();
  if (result.contains("error")) {
    QMessageBox::warning(&parent, QObject::tr("Import error"),
                         result.value("error").toString());
    return std::nullopt;
  }
  return result;
}

// e.g. a key, measure number, or tempo; the last change at or before time,
// or nullopt if there isn't one
auto get_most_recent(const QMap<int, int>& changes, const int time)
    -> std::optional<int> {
  auto iterator = changes.upperBound(time);
  if (iterator == changes.cbegin()) {
    return std::nullopt;
  }
  return std::prev(iterator).value();
}

void warn_import_error(QWidget& parent, const QString& message) {
  QMessageBox::warning(&parent, QObject::tr("Import error"), message);
}

// of a [numerator, denominator] fraction of quarter notes
auto get_denominator(const QJsonValue& time_value) -> qint64 {
  return time_value.toArray().at(1).toInteger(1);
}

// converts times to divisions per quarter note, noting if any doesn't fit in
// an int
struct TimeConverter {
  qint64 divisions = 1;
  bool overflowed = false;

  // the fewest divisions per quarter note that every time fits into
  explicit TimeConverter(const QJsonObject& result) {
    const auto add_time = [this](const QJsonValue& time_value) {
      divisions = std::lcm(divisions, get_denominator(time_value));
      if (divisions > std::numeric_limits<int>::max()) {
        overflowed = true;
        divisions = 1;
      }
    };
    const auto add_change_times = [&add_time](const QJsonValue& changes_value) {
      for (const auto& change_value : changes_value.toArray()) {
        add_time(change_value.toArray().at(0));
      }
    };
    for (const auto& note_value : result.value("notes").toArray()) {
      const auto note_object = note_value.toObject();
      add_time(note_object.value("time"));
      add_time(note_object.value("duration"));
    }
    for (const auto& staff_value : result.value("staves").toArray()) {
      add_change_times(staff_value.toObject().value("key_signatures"));
    }
    add_change_times(result.value("measure_numbers"));
    add_change_times(result.value("tempos"));
  }

  auto operator()(const QJsonValue& time_value) -> int {
    static const qint64 MAX_TIME = std::numeric_limits<int>::max();
    const auto numerator = time_value.toArray().at(0).toInteger();
    const auto scale = divisions / get_denominator(time_value);
    if (numerator > MAX_TIME || numerator * scale > MAX_TIME) {
      overflowed = true;
      return 0;
    }
    return static_cast<int>(numerator * scale);
  }
};

// [time, value] pairs, keyed by time
auto get_changes(TimeConverter& to_divisions, const QJsonValue& changes_value)
    -> QMap<int, double> {
  QMap<int, double> changes;
  for (const auto& change_value : changes_value.toArray()) {
    const auto change = change_value.toArray();
    changes[to_divisions(change.at(0))] = change.at(1).toDouble();
  }
  return changes;
}

// each instrument in each part gets its own voice
struct VoiceNumbers {
  QList<MusicXMLVoice> voices;
  // describes each voice, for its notes' words
  QList<QString> voice_words;
  QMap<std::pair<int, QString>, int> numbers;

  // a voice is named for its instrument, or failing that, its part
  auto get_voice_number(const QJsonObject& staff_object,
                        const QJsonObject& note_object) -> int {
    const auto part_name = staff_object.value("part_name").toString();
    const auto instrument_name =
        note_object.value("instrument_name").toString();
    const auto key = std::make_pair(staff_object.value("part_number").toInt(),
                                    instrument_name);
    const auto iterator = numbers.constFind(key);
    if (iterator != numbers.cend()) {
      return iterator.value();
    }
    const auto voice_number = static_cast<int>(voices.size());
    numbers.insert(key, voice_number);

    auto words = QObject::tr("Part ") + part_name;
    if (!instrument_name.isEmpty()) {
      words += QObject::tr(" instrument ") + instrument_name;
    }
    voice_words.push_back(words);

    MusicXMLVoice voice;
    voice.name = instrument_name.isEmpty() ? part_name : instrument_name;
    const auto midi_program_value = note_object.value("midi_program");
    if (midi_program_value.isDouble()) {
      voice.midi_program = MidiProgram{
          .bank_number = std::nullopt,
          .preset_number = static_cast<short>(midi_program_value.toInt())};
    }
    voices.push_back(std::move(voice));
    return voice_number;
  }
};

// Justly has one key at a time, so every pitched staff has to be in the same
// key whenever any of them changes; warns and returns nullopt if they aren't
auto get_midi_keys(QWidget& parent, const QList<QMap<int, int>>& staff_keys,
                   const QMap<int, int>& measure_numbers)
    -> std::optional<QMap<int, int>> {
  std::set<int> times;
  for (const auto& keys : staff_keys) {
    for (const auto time : keys.keys()) {
      times.insert(time);
    }
  }
  QMap<int, int> midi_keys;
  for (const auto time : times) {
    const auto midi_key = get_most_recent(staff_keys.first(), time);
    for (const auto& keys : staff_keys) {
      if (get_most_recent(keys, time) != midi_key) {
        warn_import_error(
            parent,
            QObject::tr("The key signatures in measure %1 don't match")
                .arg(get_most_recent(measure_numbers, time).value_or(1)));
        return std::nullopt;
      }
    }
    midi_keys[time] = midi_key.value();
  }
  return midi_keys;
}

// warns and returns nullopt if the score doesn't fit in Justly
auto parse_result(QWidget& parent, const QJsonObject& result)
    -> std::optional<ParsedScore> {
  static const auto FIFTH_HALFSTEPS = 7;
  static const auto MAX_MIDI_NUMBER = 127;

  ParsedScore score;
  TimeConverter to_divisions(result);
  const auto measure_numbers =
      get_changes(to_divisions, result.value("measure_numbers"));
  for (auto iterator = measure_numbers.cbegin();
       iterator != measure_numbers.cend(); ++iterator) {
    score.measure_numbers[iterator.key()] = to_int(iterator.value());
  }
  const auto tempos = get_changes(to_divisions, result.value("tempos"));
  for (auto iterator = tempos.cbegin(); iterator != tempos.cend(); ++iterator) {
    const auto tempo = to_int(iterator.value());
    if (tempo > 0) {
      score.tempos[iterator.key()] = tempo;
    }
  }

  const auto staves = result.value("staves").toArray();
  // unpitched staves' keys don't matter
  QSet<int> pitched_staff_numbers;
  VoiceNumbers pitched_voices;
  VoiceNumbers unpitched_voices;
  for (const auto& note_value : result.value("notes").toArray()) {
    const auto note_object = note_value.toObject();
    const auto time = to_divisions(note_object.value("time"));
    const auto staff_number = note_object.value("staff").toInt();
    const auto is_pitched = note_object.value("is_pitched").toBool();
    auto& voice_numbers = is_pitched ? pitched_voices : unpitched_voices;
    ParsedNote note;
    note.duration = to_divisions(note_object.value("duration"));
    note.voice_number = voice_numbers.get_voice_number(
        staves.at(staff_number).toObject(), note_object);
    note.words = voice_numbers.voice_words.at(note.voice_number);
    auto& chord = score.chords[time];
    if (is_pitched) {
      pitched_staff_numbers.insert(staff_number);
      // checked as a double, since toInt is 0 for anything too big for an int
      const auto midi_number = note_object.value("midi_number").toDouble();
      if (midi_number < 0 || midi_number > MAX_MIDI_NUMBER) {
        warn_import_error(
            parent,
            QObject::tr("A note in measure %1 is outside the midi range")
                .arg(get_most_recent(score.measure_numbers, time).value_or(1)));
        return std::nullopt;
      }
      note.midi_number = static_cast<int>(midi_number);
      note.septimal_quartertones =
          note_object.value("septimal_quartertones").toInt();
      chord.pitched_notes.push_back(std::move(note));
    } else {
      chord.unpitched_notes.push_back(std::move(note));
    }
  }
  score.voices.pitched = std::move(pitched_voices.voices);
  score.voices.unpitched = std::move(unpitched_voices.voices);

  QList<QMap<int, int>> staff_keys;
  for (auto staff_number = 0; staff_number < staves.size(); ++staff_number) {
    if (!pitched_staff_numbers.contains(staff_number)) {
      continue;
    }
    const auto sharps_changes =
        get_changes(to_divisions,
                    staves.at(staff_number).toObject().value("key_signatures"));
    QMap<int, int> keys;
    for (auto iterator = sharps_changes.cbegin();
         iterator != sharps_changes.cend(); ++iterator) {
      keys[iterator.key()] =
          MIDDLE_C_MIDI + std::get<1>(get_octave_degree(
                              FIFTH_HALFSTEPS * to_int(iterator.value())));
    }
    staff_keys.push_back(std::move(keys));
  }

  if (to_divisions.overflowed) {
    warn_import_error(
        parent, QObject::tr("The score is too long, or its rhythms too fine"));
    return std::nullopt;
  }
  score.song_divisions = static_cast<int>(to_divisions.divisions);

  auto maybe_midi_keys =
      get_midi_keys(parent, staff_keys, score.measure_numbers);
  if (!maybe_midi_keys.has_value()) {
    return std::nullopt;
  }
  score.midi_keys = std::move(maybe_midi_keys.value());
  return score;
}

auto get_interval(const int midi_interval, const int septimal_quartertones = 0)
    -> Interval {
  // Johnston's 7 lowers by 36/35, taking a 9/5 minor seventh to a 7/4
  // harmonic seventh; his el raises by the same amount
  static const auto SEPTIMAL_QUARTERTONE = Rational(36, 35);
  const auto [octave, degree] = get_octave_degree(midi_interval);
  auto ratio = get_just_scale()[degree].ratio;
  if (septimal_quartertones > 0) {
    ratio = ratio * SEPTIMAL_QUARTERTONE;
  } else if (septimal_quartertones < 0) {
    ratio = ratio / SEPTIMAL_QUARTERTONE;
  }
  return Interval(ratio, octave);
}

auto get_max_duration(const QList<ParsedNote>& notes) -> int {
  if (notes.empty()) {
    return 0;
  }
  return std::ranges::max_element(notes,
                                  [](const ParsedNote& first_note,
                                     const ParsedNote& second_note) -> auto {
                                    return first_note.duration <
                                           second_note.duration;
                                  })
      ->duration;
}

// pitched notes are relative to the chord's key
template <NoteInterface SubNote>
auto make_notes(const QList<ParsedNote>& parse_notes,
                const QList<MusicXMLVoice>& voices, const int key,
                const int song_divisions) -> QList<SubNote> {
  QList<SubNote> new_notes;
  for (const auto& parse_note : parse_notes) {
    SubNote new_note;
    new_note.beats = Rational(parse_note.duration, song_divisions);
    new_note.words = parse_note.words;
    new_note.voice_name = voices.at(parse_note.voice_number).name;
    if constexpr (std::same_as<SubNote, PitchedNote>) {
      new_note.interval = get_interval(parse_note.midi_number - key,
                                       parse_note.septimal_quartertones);
    }
    new_notes.push_back(std::move(new_note));
  }
  return new_notes;
}

auto make_chord(const ParsedChord& parse_chord, const MusicXMLVoices& voices,
                const int measure_number, const int key,
                const int last_midi_key, const int tempo, const int last_tempo,
                const int song_divisions, const int time_delta) -> Chord {
  Chord new_chord;
  new_chord.beats = Rational(time_delta, song_divisions);
  new_chord.interval = get_interval(key - last_midi_key);
  new_chord.tempo_ratio = Rational(tempo, last_tempo);
  new_chord.words = QString::number(measure_number);
  new_chord.pitched_notes = make_notes<PitchedNote>(
      parse_chord.pitched_notes, voices.pitched, key, song_divisions);
  new_chord.unpitched_notes = make_notes<UnpitchedNote>(
      parse_chord.unpitched_notes, voices.unpitched, key, song_divisions);
  return new_chord;
}

auto deduplicate_voice_names(QList<MusicXMLVoice> voices)
    -> QList<MusicXMLVoice> {
  QSet<QString> used_names;
  for (auto& voice : voices) {
    auto& voice_name = voice.name;
    if (voice_name.isEmpty()) {
      voice_name = QObject::tr("Unnamed instrument");
    }
    auto candidate_name = voice_name;
    for (auto suffix_number = 2; used_names.contains(candidate_name);
         ++suffix_number) {
      candidate_name = voice_name + QString(" (%1)").arg(suffix_number);
    }
    voice_name = candidate_name;
    used_names.insert(candidate_name);
  }
  return voices;
}

}  // namespace

auto import_score(QWidget& parent, const QString& filename)
    -> std::optional<ImportedScore> {
  const auto maybe_result = run_converter(parent, filename);
  if (!maybe_result.has_value()) {
    return std::nullopt;
  }
  auto maybe_parsed_score = parse_result(parent, maybe_result.value());
  if (!maybe_parsed_score.has_value()) {
    return std::nullopt;
  }
  auto& parsed_score = maybe_parsed_score.value();
  const auto song_divisions = parsed_score.song_divisions;
  auto& voices = parsed_score.voices;
  const auto& chords_dict = parsed_score.chords;
  const auto& midi_keys = parsed_score.midi_keys;
  const auto& measure_numbers = parsed_score.measure_numbers;
  const auto& tempos = parsed_score.tempos;

  if (chords_dict.empty()) {
    QMessageBox::warning(&parent, QObject::tr("Empty MusicXML error"),
                         QObject::tr("No chords"));
    return std::nullopt;
  }

  if (voices.unpitched.empty()) {
    // a file with no percussion/unpitched notes would otherwise leave
    // song.unpitched_voices completely empty, so manually inserting any
    // unpitched note afterward (which defaults to the first voice) would
    // reference a voice that doesn't exist
    MusicXMLVoice default_voice;
    default_voice.name = QObject::tr("unpitched voice 1");
    voices.unpitched.push_back(std::move(default_voice));
  }

  ImportedScore score;
  score.voices.pitched = deduplicate_voice_names(voices.pitched);
  score.voices.unpitched = deduplicate_voice_names(voices.unpitched);
  score.starting_midi_key = get_most_recent(midi_keys, chords_dict.firstKey())
                                .value_or(DEFAULT_STARTING_MIDI);
  score.starting_tempo = get_most_recent(tempos, chords_dict.firstKey())
                             .value_or(DEFAULT_STARTING_TEMPO);

  auto last_midi_key = score.starting_midi_key;
  auto last_tempo = score.starting_tempo;
  for (auto iterator = chords_dict.cbegin(); iterator != chords_dict.cend();
       ++iterator) {
    const auto time = iterator.key();
    const auto& chord = iterator.value();
    const auto next_iterator = std::next(iterator);
    const auto midi_key =
        get_most_recent(midi_keys, time).value_or(DEFAULT_STARTING_MIDI);
    const auto tempo =
        get_most_recent(tempos, time).value_or(DEFAULT_STARTING_TEMPO);
    score.chords.push_back(make_chord(
        chord, score.voices, get_most_recent(measure_numbers, time).value_or(1),
        midi_key, last_midi_key, tempo, last_tempo, song_divisions,
        next_iterator == chords_dict.cend()
            ? std::max(get_max_duration(chord.pitched_notes),
                       get_max_duration(chord.unpitched_notes))
            : next_iterator.key() - time));
    last_midi_key = midi_key;
    last_tempo = tempo;
  }
  return score;
}
