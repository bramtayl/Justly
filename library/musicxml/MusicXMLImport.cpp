#include "musicxml/MusicXMLImport.hpp"

#include <QtCore/QSet>
#include <QtWidgets/QMessageBox>

#include "other/Song.hpp"
#include "xml/XMLChildren.hpp"
#include "xml/ZipArchive.hpp"

namespace {

auto maybe_read_compressed_musicxml_bytes(const QString& filename)
    -> QByteArray {
  const ZipArchive archive(filename);
  if (archive.internal_pointer == nullptr) {
    return {};
  }

  const auto container_bytes =
      read_zip_entry(archive, "META-INF/container.xml");
  if (container_bytes.isEmpty()) {
    return {};
  }

  const auto container_document = read_xml_document(container_bytes);
  if (container_document.internal_pointer == nullptr) {
    return {};
  }

  auto* rootfiles_pointer =
      maybe_get_xml_child(get_root(container_document), "rootfiles");
  auto* rootfile_pointer =
      rootfiles_pointer == nullptr
          ? nullptr
          : maybe_get_xml_child(get_reference(rootfiles_pointer), "rootfile");
  if (rootfile_pointer == nullptr) {
    return {};
  }

  const auto root_path =
      get_property(get_reference(rootfile_pointer), "full-path");

  return read_zip_entry(archive, root_path);
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

auto get_max_duration(const QList<MusicXMLNote>& notes) -> int {
  if (notes.empty()) {
    return 0;
  }
  return std::ranges::max_element(notes,
                                  [](const MusicXMLNote& first_note,
                                     const MusicXMLNote& second_note) -> auto {
                                    return first_note.duration <
                                           second_note.duration;
                                  })
      ->duration;
}

// pitched notes are relative to the chord's key
template <NoteInterface SubNote>
auto make_notes(const QList<MusicXMLNote>& parse_notes,
                const QList<QString>& voice_names, const int key,
                const int song_divisions) -> QList<SubNote> {
  QList<SubNote> new_notes;
  for (const auto& parse_note : parse_notes) {
    SubNote new_note;
    new_note.beats = Rational(parse_note.duration, song_divisions);
    new_note.words = parse_note.words;
    new_note.voice_name = voice_names.at(parse_note.voice_number);
    if constexpr (std::same_as<SubNote, PitchedNote>) {
      new_note.interval = get_interval(parse_note.midi_number - key,
                                       parse_note.septimal_quartertones);
    }
    new_notes.push_back(std::move(new_note));
  }
  return new_notes;
}

auto make_chord(const MusicXMLChord& parse_chord, const VoiceNames& voice_names,
                const int measure_number, const int key,
                const int last_midi_key, const int song_divisions,
                const int time_delta) -> Chord {
  Chord new_chord;
  new_chord.beats = Rational(time_delta, song_divisions);
  new_chord.interval = get_interval(key - last_midi_key);
  new_chord.words = QString::number(measure_number);
  new_chord.pitched_notes = make_notes<PitchedNote>(
      parse_chord.pitched_notes, voice_names.pitched, key, song_divisions);
  new_chord.unpitched_notes = make_notes<UnpitchedNote>(
      parse_chord.unpitched_notes, voice_names.unpitched, key, song_divisions);
  return new_chord;
}

auto deduplicate_voice_names(QList<QString> voice_names) -> QList<QString> {
  QSet<QString> used_names;
  for (auto& voice_name : voice_names) {
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
  return voice_names;
}

}  // namespace

auto read_musicxml_document(const QString& filename) -> XMLDocument {
  if (filename.endsWith(".mxl", Qt::CaseInsensitive)) {
    return read_xml_document(maybe_read_compressed_musicxml_bytes(filename));
  }
  return read_xml_file(filename);
}

auto import_score(QWidget& parent, xmlNode& score_partwise)
    -> std::optional<ImportedScore> {
  if (!node_is(score_partwise, "score-partwise")) {
    QMessageBox::warning(
        &parent, QObject::tr("Partwise error"),
        QObject::tr("Justly only supports partwise musicxml scores"));
    return std::nullopt;
  }

  auto maybe_parsed_score = parse_score(parent, score_partwise);
  if (!maybe_parsed_score.has_value()) {
    return std::nullopt;
  }
  auto& parsed_score = maybe_parsed_score.value();
  const auto song_divisions = parsed_score.song_divisions;
  auto& voice_names = parsed_score.voice_names;
  const auto& chords_dict = parsed_score.chords;
  const auto& midi_keys = parsed_score.midi_keys;
  const auto& measure_numbers = parsed_score.measure_numbers;

  if (chords_dict.empty()) {
    QMessageBox::warning(&parent, QObject::tr("Empty MusicXML error"),
                         QObject::tr("No chords"));
    return std::nullopt;
  }

  if (voice_names.unpitched.empty()) {
    // a file with no percussion/unpitched notes would otherwise leave
    // song.unpitched_voices completely empty, so manually inserting any
    // unpitched note afterward (which defaults to the first voice) would
    // reference a voice that doesn't exist
    voice_names.unpitched.push_back(QObject::tr("unpitched voice 1"));
  }

  ImportedScore score;
  score.voice_names.pitched = deduplicate_voice_names(voice_names.pitched);
  score.voice_names.unpitched = deduplicate_voice_names(voice_names.unpitched);
  score.starting_midi_key =
      get_most_recent(midi_keys, chords_dict.firstKey(), DEFAULT_STARTING_MIDI);

  auto last_midi_key = score.starting_midi_key;
  for (auto iterator = chords_dict.cbegin(); iterator != chords_dict.cend();
       ++iterator) {
    const auto time = iterator.key();
    const auto& chord = iterator.value();
    const auto next_iterator = std::next(iterator);
    const auto midi_key =
        get_most_recent(midi_keys, time, DEFAULT_STARTING_MIDI);
    score.chords.push_back(make_chord(
        chord, score.voice_names, get_most_recent(measure_numbers, time, 1),
        midi_key, last_midi_key, song_divisions,
        next_iterator == chords_dict.cend()
            ? std::max(get_max_duration(chord.pitched_notes),
                       get_max_duration(chord.unpitched_notes))
            : next_iterator.key() - time));
    last_midi_key = midi_key;
  }
  return score;
}
