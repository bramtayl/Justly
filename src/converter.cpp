#include <QtCore/QObject>
#include <cerrno>
#include <cstdlib>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <tinyxml2.h>
#include <vector>

static const auto CONCERT_A_FREQUENCY = 440;
static const auto CONCERT_A_MIDI = 69;
static const auto DEFAULT_STARTING_KEY = 220;
static const auto DEFAULT_STARTING_TEMPO = 100;
static const auto HALFSTEPS_PER_OCTAVE = 12;
static const auto OCTAVE_RATIO = 2.0;

struct Rational {
  int numerator = 1;
  int denominator = 1;

  explicit Rational(const int numerator_input = 1,
                    const int denominator_input = 1) {
    Q_ASSERT(denominator_input != 0);
    const auto common_denominator =
        std::gcd(numerator_input, denominator_input);
    numerator = numerator_input / common_denominator;
    denominator = denominator_input / common_denominator;
  }

  [[nodiscard]] auto to_double() const -> double {
    Q_ASSERT(denominator != 0);
    return 1.0 * numerator / denominator;
  }

  [[nodiscard]] auto operator+(const Rational &other_rational) const {
    auto other_denominator = other_rational.denominator;
    return Rational(numerator * other_denominator +
                        denominator * other_rational.numerator,
                    denominator * other_denominator);
  }

  [[nodiscard]] auto operator<(const Rational &other_interval) const {
    return to_double() < other_interval.to_double();
  }
};

struct Interval {
  int numerator = 1;
  int denominator = 1;
  int octave = 0;

  explicit Interval(const int numerator_input = 1,
                    const int denominator_input = 1,
                    const int octave_input = 0) {
    Q_ASSERT(denominator_input != 0);
    const auto rational = Rational(numerator_input, denominator_input);
    numerator = rational.numerator;
    denominator = denominator = rational.denominator;
    octave = octave_input;
  }
};

struct ParsePitchedNote {
  int duration = 0;
  int midi_number = 0;
  std::string words;
};

struct PitchedNote {
  Rational beats;
  Interval interval;
  std::string words;
};

struct ParseUnpitchedNote {
  int duration = 0;
  std::string words;
};

struct UnpitchedNote {
  Rational beats;
  std::string words;
};

struct PitchedNoteInfo {
  ParsePitchedNote pitched_note;
  int start_time = 0;
};

struct ParseChord {
  int midi_key = 0;
  int divisions = 0;
  int duration = 0;
  std::vector<ParsePitchedNote> pitched_notes;
  std::vector<ParseUnpitchedNote> unpitched_notes;
  int measure_number = 0;
};

struct Chord {
  Rational beats;
  Rational tempo_ratio;
  Interval interval;
  std::vector<PitchedNote> pitched_notes;
  std::vector<UnpitchedNote> unpitched_notes;
  std::string words;
};

// TODO(brandon): just iterate once
[[nodiscard]] static auto
get_current_value(const std::map<int, int> &event_dict, const int current_time,
                  const int default_value) -> int {
  auto event_iterator = event_dict.begin();
  if (event_iterator == event_dict.end()) {
    return default_value;
  }
  if (event_iterator->first > current_time) {
    return default_value;
  }
  int last_event_value = event_iterator->second;
  ++event_iterator;
  while (event_iterator != event_dict.end()) {
    if (event_iterator->first > current_time) {
      break;
    }
    last_event_value = event_iterator->second;
    ++event_iterator;
  }
  return last_event_value;
}

struct PartInfo {
  std::string default_instrument_id;
  std::map<std::string, std::string> instrument_map;
};

struct Song {
  double starting_key = DEFAULT_STARTING_KEY;
  double starting_tempo = DEFAULT_STARTING_TEMPO;
  std::vector<Chord> chords;
};

template <typename Thing>
[[nodiscard]] static auto get_reference(Thing *thing_pointer) -> Thing & {
  Q_ASSERT(thing_pointer != nullptr);
  return *thing_pointer;
}

[[nodiscard]] static auto
get_duration(const tinyxml2::XMLElement &measure_element) -> int {
  return std::stoi(
      get_reference(measure_element.FirstChildElement("duration")).GetText());
}

[[nodiscard]] static auto get_interval(const int midi_interval) -> Interval {
  const auto octave = midi_interval / HALFSTEPS_PER_OCTAVE;
  static const std::vector<Rational> scale = {
      Rational(1, 1), Rational(16, 15), Rational(9, 8),   Rational(6, 5),
      Rational(5, 4), Rational(4, 3),   Rational(45, 32), Rational(3, 2),
      Rational(8, 5), Rational(5, 3),   Rational(9, 5),   Rational(15, 8)};
  const auto &pitch = scale[midi_interval - octave * HALFSTEPS_PER_OCTAVE];
  return Interval(pitch.numerator, pitch.denominator, octave);
}

static void add_chord(std::vector<Chord> &chords, const ParseChord &parse_chord,
                      const Interval &interval, const Rational &tempo_ratio,
                      const int time_delta) {
  const auto midi_key = parse_chord.midi_key;
  const auto divisions = parse_chord.divisions;
  Chord new_chord;
  new_chord.beats = Rational(time_delta, divisions);
  new_chord.interval = interval;
  new_chord.words = std::to_string(parse_chord.measure_number);
  new_chord.tempo_ratio = tempo_ratio;
  auto &unpitched_notes = new_chord.unpitched_notes;
  for (const auto &parse_unpitched_note : parse_chord.unpitched_notes) {
    UnpitchedNote new_unpitched_note;
    new_unpitched_note.beats =
        Rational(parse_unpitched_note.duration / divisions);
    new_unpitched_note.words = parse_unpitched_note.words;
    unpitched_notes.push_back(std::move(new_unpitched_note));
  }
  auto &pitched_notes = new_chord.pitched_notes;
  for (const auto &parse_pitched_note : parse_chord.pitched_notes) {
    PitchedNote new_pitched_note;
    new_pitched_note.beats = Rational(parse_pitched_note.duration / divisions);
    new_pitched_note.words = parse_pitched_note.words;
    new_pitched_note.interval =
        get_interval(parse_pitched_note.midi_number - midi_key);
    pitched_notes.push_back(std::move(new_pitched_note));
  }
  chords.push_back(std::move(new_chord));
}

static void
add_pitched_note_and_maybe_chord(std::map<int, ParseChord> &part_chord_dict,
                                 const int start_time,
                                 ParsePitchedNote pitched_note) {
  if (part_chord_dict.contains(start_time)) {
    part_chord_dict.at(start_time)
        .pitched_notes.push_back(std::move(pitched_note));
  } else {
    ParseChord new_chord;
    new_chord.pitched_notes.push_back(std::move(pitched_note));
    part_chord_dict[start_time] = std::move(new_chord);
  }
}

[[nodiscard]] static auto
get_instrument_id(const std::string &default_id,
                  const tinyxml2::XMLElement &measure_element) -> std::string {
  const auto *instrument_pointer =
      measure_element.FirstChildElement("instrument");
  if (instrument_pointer != nullptr) {
    return get_reference(instrument_pointer).Attribute("id");
  }
  return default_id;
}

template <typename NoteType>
[[nodiscard]] static auto
get_max_duration(std::vector<NoteType> &notes) -> int {
  if (notes.empty()) {
    return 0;
  }
  return std::max_element(
             notes.begin(), notes.end(),
             [](const NoteType &first_note, const NoteType &second_note) {
               return first_note.duration < second_note.duration;
             })
      ->duration;
}

// TODO(brandon): validate musicxml
[[nodiscard]] auto parse_file(const std::string &filename) -> Song {
  const std::map<int, int> major_key_to_midi = {
      {-7, 53}, {-6, 55}, {-5, 57}, {-4, 58}, {-3, 60}, {-2, 62},
      {-1, 64}, {0, 65},  {1, 67},  {2, 69},  {3, 71},  {4, 72},
      {5, 74},  {6, 76},  {7, 77}}; // TODO(brandon): check numbers

  static const std::map<std::string, int> note_to_midi = {
      {"C", 0},  {"C#", 1}, {"Db", 1},  {"D", 2},   {"D#", 3}, {"Eb", 3},
      {"E", 4},  {"F", 5},  {"F#", 6},  {"Gb", 6},  {"G", 7},  {"G#", 8},
      {"Ab", 8}, {"A", 9},  {"A#", 10}, {"Bb", 10}, {"B", 11}};

  tinyxml2::XMLDocument document;
  const auto result = document.LoadFile(filename.c_str());
  Q_ASSERT(result == tinyxml2::XML_SUCCESS);

  // Get root measure_element
  const auto *root_pointer = document.RootElement();
  Q_ASSERT(root_pointer != nullptr);
  if (std::string(root_pointer->Name()) != "score-partwise") {
    // TODO(brandon): QT error
    Q_ASSERT(false);
  };
  const auto &root = *root_pointer;

  std::map<std::string, PartInfo> part_info_dict;
  const auto *score_part_pointer = root.FirstChildElement("score-part");
  while (score_part_pointer != nullptr) {
    const auto &score_part = get_reference(score_part_pointer);

    PartInfo part_info;
    auto &instrument_map = part_info.instrument_map;
    const auto *instrument_pointer =
        score_part.FirstChildElement("score-instrument");
    bool first_one = true;
    while (instrument_pointer != nullptr) {
      const auto &score_instrument = get_reference(instrument_pointer);
      const std::string instrument_id = score_instrument.Attribute("id");
      if (first_one) {
        part_info.default_instrument_id = instrument_id;
        first_one = false;
      }
      instrument_map[instrument_id] =
          get_reference(score_instrument.FirstChildElement("instrument-name"))
              .GetText();
      instrument_pointer =
          score_instrument.NextSiblingElement("score-instrument");
    }
    part_info_dict[score_part.Attribute("id")] = std::move(part_info);
    score_part_pointer = score_part.NextSiblingElement("score-part");
  }

  std::map<int, ParseChord> chords_dict;

  const auto *part_pointer = root.FirstChildElement("part");
  while (part_pointer != nullptr) {
    auto current_time = 0;
    auto chord_start_time = current_time;
    auto measure_number = 1;

    std::map<int, PitchedNoteInfo> tied_pitched_notes_info;

    const auto &part = get_reference(part_pointer);
    const std::string part_id = part.Attribute("id");
    auto &part_info = part_info_dict[part_id];
    auto &instrument_map = part_info.instrument_map;
    auto &default_instrument_id = part_info.default_instrument_id;

    std::map<int, ParseChord> part_chord_dict;
    std::map<int, int> keys_dict;
    std::map<int, int> divisions_dict;
    std::map<int, int> measure_number_dict;

    const auto *measure_pointer = part.FirstChildElement("measure");

    while (measure_pointer != nullptr) {
      const auto &measure = get_reference(measure_pointer);
      measure_number_dict[current_time] = measure_number;
      const auto *measure_element_pointer = measure.FirstChildElement();
      while (measure_element_pointer != nullptr) {
        const auto &measure_element = get_reference(measure_element_pointer);
        const std::string measure_element_name = measure_element.Name();
        const auto *pitch_pointer = measure_element.FirstChildElement("pitch");
        if (measure_element_name == "attributes") {
          const auto *key_pointer = measure_element.FirstChildElement("key");
          if (key_pointer != nullptr) {
            keys_dict[current_time] = std::stoi(
                get_reference(
                    get_reference(key_pointer).FirstChildElement("fifths"))
                    .GetText());
          }
          const auto *divisions_pointer =
              measure_element.FirstChildElement("divisions");
          if (divisions_pointer != nullptr) {
            divisions_dict[current_time] =
                std::stoi(get_reference(divisions_pointer).GetText());
          }
        } else if (measure_element_name == "note") {
          const auto note_duration = get_duration(measure_element);
          if (measure_element_pointer->FirstChildElement("chord") == nullptr) {
            chord_start_time = current_time;
            current_time = current_time + note_duration;
          }

          if (measure_element.FirstChildElement("unpitched") != nullptr) {
            ParseUnpitchedNote new_unpitched_note;
            new_unpitched_note.words = instrument_map[get_instrument_id(
                default_instrument_id, measure_element)];
            new_unpitched_note.duration = note_duration;
            if (part_chord_dict.contains(chord_start_time)) {
              auto &existing_chord = part_chord_dict.at(chord_start_time);
              existing_chord.unpitched_notes.push_back(
                  std::move(new_unpitched_note));
            } else {
              ParseChord new_chord;
              new_chord.unpitched_notes.push_back(
                  std::move(new_unpitched_note));
              part_chord_dict[chord_start_time] = std::move(new_chord);
            }
          } else if (pitch_pointer != nullptr) {
            bool tie_start = false;
            bool tie_end = false;

            const auto *tie_pointer = measure_element.FirstChildElement("tie");
            while (tie_pointer != nullptr) {
              const auto &tie_element = get_reference(tie_pointer);
              const std::string tie_type = tie_element.Attribute("type");
              if (tie_type == "stop") {
                tie_end = true;
              } else if (tie_type == "start") {
                tie_start = true;
              };
              tie_pointer = tie_element.NextSiblingElement("tie");
            }

            const auto &pitch = get_reference(pitch_pointer);
            const auto &step = get_reference(pitch.FirstChildElement("step"));
            const auto &octave =
                get_reference(pitch.FirstChildElement("octave"));

            const auto *alteration_pointer = pitch.FirstChildElement("alter");
            auto alteration = 0;
            if (alteration_pointer != nullptr) {
              alteration =
                  std::stoi(get_reference(alteration_pointer).GetText());
            }

            auto note_midi_number =
                note_to_midi.at(step.GetText()) +
                (std::stoi(octave.GetText()) + 1) * HALFSTEPS_PER_OCTAVE +
                alteration;
            if (tie_end) {
              const auto note_info_iterator =
                  tied_pitched_notes_info.find(note_midi_number);
              Q_ASSERT(note_info_iterator != tied_pitched_notes_info.end());
              auto &previous_tie_info = note_info_iterator->second;
              auto &previous_note = previous_tie_info.pitched_note;
              previous_note.duration = previous_note.duration + note_duration;
              if (!tie_start) {
                add_pitched_note_and_maybe_chord(part_chord_dict,
                                                 previous_tie_info.start_time,
                                                 std::move(previous_note));
                tied_pitched_notes_info.erase(note_info_iterator);
              }
            } else {
              ParsePitchedNote new_pitched_note;
              new_pitched_note.duration = note_duration;
              new_pitched_note.words = instrument_map[get_instrument_id(
                  default_instrument_id, measure_element)];
              new_pitched_note.midi_number = note_midi_number;
              if (tie_start) { // also not tie end
                tied_pitched_notes_info[note_midi_number] = PitchedNoteInfo(
                    {std::move(new_pitched_note), chord_start_time});
              } else { // not tie start or end
                add_pitched_note_and_maybe_chord(part_chord_dict,
                                                 chord_start_time,
                                                 std::move(new_pitched_note));
              }
            }
          }
        } else if (measure_element_name == "backup") {
          current_time = current_time - get_duration(measure_element);
          chord_start_time = current_time;
        } else if (measure_element_name == "forward") {
          current_time = current_time + get_duration(measure_element);
          chord_start_time = current_time;
        }
        measure_element_pointer = measure_element.NextSiblingElement();
      }
      measure_number = measure_number + 1;
      measure_pointer = measure.NextSiblingElement("measure");
    }
    Q_ASSERT(tied_pitched_notes_info.empty());
    auto part_chord_iterator = part_chord_dict.begin();
    while (part_chord_iterator != part_chord_dict.end()) {
      const auto chord_time = part_chord_iterator->first;
      auto &chord = part_chord_iterator->second;
      chord.measure_number =
          get_current_value(measure_number_dict, chord_time, -1);
      chord.midi_key =
          major_key_to_midi.at(get_current_value(keys_dict, chord_time, 0));
      chord.divisions = get_current_value(divisions_dict, chord_time, 1);
      chords_dict[chord_time] = chord;
      ++part_chord_iterator;
    }
    part_pointer = part.NextSiblingElement("part");
  }
  Song song;

  auto &chords = song.chords;

  auto chord_iterator = chords_dict.begin();
  if (chord_iterator == chords_dict.end()) {
    Q_ASSERT(false); // TODO(brandon): qt warnnig
    return {};
  }

  int last_time = chord_iterator->first;
  auto last_parse_chord = chord_iterator->second;
  auto last_midi_key = last_parse_chord.midi_key;
  auto last_divisions = last_parse_chord.divisions;
  auto last_interval = get_interval(0);
  auto last_tempo_ratio = Rational(1, 1);
  song.starting_key =
      pow(OCTAVE_RATIO,
          (1.0 * (last_midi_key - CONCERT_A_MIDI)) / HALFSTEPS_PER_OCTAVE) *
      CONCERT_A_FREQUENCY;

  ++chord_iterator;
  while (chord_iterator != chords_dict.end()) {
    int time = chord_iterator->first;
    add_chord(chords, last_parse_chord, last_interval, last_tempo_ratio,
              time - last_time);
    auto parse_chord = chord_iterator->second;
    const auto current_midi_key = parse_chord.midi_key;
    const auto divisions = parse_chord.divisions;
    last_interval = get_interval(current_midi_key - last_midi_key);
    last_tempo_ratio = Rational(last_divisions, divisions);
    last_time = time;
    last_midi_key = current_midi_key;
    last_divisions = divisions;
    last_parse_chord = parse_chord;
    ++chord_iterator;
  }
  add_chord(chords, last_parse_chord, last_interval, last_tempo_ratio,
            std::max(get_max_duration(last_parse_chord.pitched_notes),
                     get_max_duration(last_parse_chord.unpitched_notes)));
  return song;
}

// TODO(brandon): specific measure names?
