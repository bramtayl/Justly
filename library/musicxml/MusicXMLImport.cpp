#include "musicxml/MusicXMLImport.hpp"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMap>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QSet>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>

#include "other/Song.hpp"
#include "other/helpers.hpp"

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
    process.start(python_program,
                  {QString::fromStdString(get_share_file("import_score.py")),
                   filename});
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
  if (process.exitStatus() != QProcess::NormalExit ||
      process.exitCode() != 0) {
    // e.g. music21 isn't installed
    QMessageBox::warning(
        &parent, QObject::tr("Python error"),
        QObject::tr("Couldn't read the score with music21:\n%1")
            .arg(QString::fromUtf8(process.readAllStandardError()).trimmed()));
    return std::nullopt;
  }
  const auto document = QJsonDocument::fromJson(process.readAllStandardOutput());
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

// each [time, value] pair, keyed by time
auto get_changes(const QJsonObject& result, const char* const field_name)
    -> QMap<int, int> {
  QMap<int, int> changes;
  for (const auto& change_value : result.value(field_name).toArray()) {
    const auto change = change_value.toArray();
    changes[change.at(0).toInt()] = change.at(1).toInt();
  }
  return changes;
}

// a voice is named for its instrument, or failing that, its part
auto get_voices(const QJsonArray& voice_values, QList<QString>& voice_words)
    -> QList<MusicXMLVoice> {
  QList<MusicXMLVoice> voices;
  for (const auto& voice_value : voice_values) {
    const auto voice_object = voice_value.toObject();
    const auto part_name = voice_object.value("part_name").toString();
    const auto instrument_name =
        voice_object.value("instrument_name").toString();
    auto words = QObject::tr("Part ") + part_name;
    if (!instrument_name.isEmpty()) {
      words += QObject::tr(" instrument ") + instrument_name;
    }
    voice_words.push_back(words);

    MusicXMLVoice voice;
    voice.name = instrument_name.isEmpty() ? part_name : instrument_name;
    const auto midi_program_value = voice_object.value("midi_program");
    if (midi_program_value.isDouble()) {
      voice.midi_program = MidiProgram{
          .bank_number = std::nullopt,
          .preset_number = static_cast<short>(midi_program_value.toInt())};
    }
    voices.push_back(std::move(voice));
  }
  return voices;
}

auto parse_result(const QJsonObject& result) -> ParsedScore {
  ParsedScore score;
  score.song_divisions = result.value("divisions").toInt(1);

  const auto voices_object = result.value("voices").toObject();
  QList<QString> pitched_words;
  QList<QString> unpitched_words;
  score.voices.pitched =
      get_voices(voices_object.value("pitched").toArray(), pitched_words);
  score.voices.unpitched =
      get_voices(voices_object.value("unpitched").toArray(), unpitched_words);

  for (const auto& note_value : result.value("notes").toArray()) {
    const auto note_object = note_value.toObject();
    const auto is_pitched = note_object.value("is_pitched").toBool();
    ParsedNote note;
    note.duration = note_object.value("duration").toInt();
    note.voice_number = note_object.value("voice").toInt();
    note.words =
        (is_pitched ? pitched_words : unpitched_words).at(note.voice_number);
    auto& chord = score.chords[note_object.value("time").toInt()];
    if (is_pitched) {
      note.midi_number = note_object.value("midi_number").toInt();
      note.septimal_quartertones =
          note_object.value("septimal_quartertones").toInt();
      chord.pitched_notes.push_back(std::move(note));
    } else {
      chord.unpitched_notes.push_back(std::move(note));
    }
  }

  score.midi_keys = get_changes(result, "midi_keys");
  score.measure_numbers = get_changes(result, "measure_numbers");
  score.tempos = get_changes(result, "tempos");
  return score;
}

// the value of the last change at or before time
auto get_most_recent(const QMap<int, int>& changes, const int time,
                     const int default_value) -> int {
  auto iterator = changes.upperBound(time);
  if (iterator == changes.cbegin()) {
    return default_value;
  }
  return std::prev(iterator).value();
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
                const int last_midi_key, const int tempo,
                const int last_tempo, const int song_divisions,
                const int time_delta) -> Chord {
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
  auto parsed_score = parse_result(maybe_result.value());
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
  score.starting_midi_key =
      get_most_recent(midi_keys, chords_dict.firstKey(), DEFAULT_STARTING_MIDI);
  score.starting_tempo = get_most_recent(tempos, chords_dict.firstKey(),
                                         DEFAULT_STARTING_TEMPO);

  auto last_midi_key = score.starting_midi_key;
  auto last_tempo = score.starting_tempo;
  for (auto iterator = chords_dict.cbegin(); iterator != chords_dict.cend();
       ++iterator) {
    const auto time = iterator.key();
    const auto& chord = iterator.value();
    const auto next_iterator = std::next(iterator);
    const auto midi_key =
        get_most_recent(midi_keys, time, DEFAULT_STARTING_MIDI);
    const auto tempo = get_most_recent(tempos, time, DEFAULT_STARTING_TEMPO);
    score.chords.push_back(make_chord(
        chord, score.voices, get_most_recent(measure_numbers, time, 1),
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
