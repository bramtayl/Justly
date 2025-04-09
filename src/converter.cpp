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
static const auto HALFSTEPS_PER_OCTAVE = 12;
static const auto MIDI_C4 = 60;
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

struct PitchedNote {
  Rational beats;
  Interval interval;
  std::string words;
  std::string pitched_instrument;
};

struct UnpitchedNote {
  Rational beats;
  std::string words;
  std::string unpitched_instrument;
};

struct PitchedNoteInfo {
  PitchedNote pitched_note;
  int measure_number = 0;
  int midi_key = 0;
  int start_time = 0;
  int divisions = 0;
};

struct Chord {
  int measure_number = 0;
  Rational beats;
  Interval interval;
  std::vector<PitchedNote> pitched_notes;
  std::vector<UnpitchedNote> unpitched_notes;
  std::string pitched_instrument;
  std::string unpitched_instrument;
  std::string words;
};

struct ChordInfo {
  Chord chord;
  int midi_key;
  int divisions;
};

struct PartInstruments {
  std::string default_instrument;
  std::map<std::string, std::string> instrument_map;
};

struct Song {
  double starting_key = 57;
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

static void
add_pitched_note_and_maybe_chord(std::map<int, ChordInfo> &chord_info_dict,
                                 const int midi_number, const int start_time,
                                 const int measure_number, const int divisions,
                                 const int midi_key, PitchedNote pitched_note) {
  pitched_note.interval = get_interval(midi_number - midi_key);
  if (chord_info_dict.contains(start_time)) {
    auto &existing_chord = chord_info_dict.at(start_time).chord;
    if (existing_chord.pitched_instrument == pitched_note.pitched_instrument) {
      pitched_note.pitched_instrument = "";
    }
    existing_chord.pitched_notes.push_back(std::move(pitched_note));
  } else {
    Chord new_chord;
    new_chord.words = std::to_string(measure_number);
    new_chord.pitched_instrument = pitched_note.pitched_instrument;
    pitched_note.pitched_instrument = "";
    new_chord.pitched_notes.push_back(std::move(pitched_note));
    chord_info_dict[start_time] =
        ChordInfo({std::move(new_chord), midi_key, divisions});
  }
}

[[nodiscard]] static auto
get_instrument(const PartInstruments &score_part,
               const tinyxml2::XMLElement &measure_element) -> std::string {
  const auto *instrument_pointer =
      measure_element.FirstChildElement("instrument");
  if (instrument_pointer != nullptr) {
    return score_part.instrument_map.at(
        get_reference(instrument_pointer).Attribute("id"));
  }
  return score_part.default_instrument;
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

  std::map<std::string, PartInstruments> part_instruments_dict;
  const auto *score_part_pointer = root.FirstChildElement("score-part");
  while (score_part_pointer != nullptr) {
    const auto &score_part = get_reference(score_part_pointer);

    PartInstruments part_instruments;
    auto &instrument_map = part_instruments.instrument_map;
    const auto *instrument_pointer =
        score_part.FirstChildElement("score-instrument");
    bool first_one = true;
    while (instrument_pointer != nullptr) {
      const auto &score_instrument = get_reference(instrument_pointer);
      std::string instrument_name =
          get_reference(score_instrument.FirstChildElement("instrument-name"))
              .GetText();
      if (first_one) {
        part_instruments.default_instrument = instrument_name;
        first_one = false;
      }
      instrument_map[score_instrument.Attribute("id")] =
          std::move(instrument_name);
      instrument_pointer =
          score_instrument.NextSiblingElement("score-instrument");
    }
    part_instruments_dict[score_part.Attribute("id")] =
        std::move(part_instruments);
    score_part_pointer = score_part.NextSiblingElement("score-part");
  }

  std::map<int, ChordInfo> chord_info_dict;

  const auto *part_pointer = root.FirstChildElement("part");
  while (part_pointer != nullptr) {
    auto current_midi_key = MIDI_C4;
    auto current_time = 0;
    auto current_divisions = 1;
    auto measure_number = 1;

    std::map<int, PitchedNoteInfo> tied_pitched_notes_info;

    const auto &part = get_reference(part_pointer);
    const auto &part_instruments = part_instruments_dict[part.Attribute("id")];
    const auto *measure_pointer = part.FirstChildElement("measure");

    while (measure_pointer != nullptr) {
      const auto &measure = get_reference(measure_pointer);
      auto chord_start_time = current_time;
      bool is_chord = false;

      const auto *measure_element_pointer = measure.FirstChildElement();
      while (measure_element_pointer != nullptr) {
        const auto &measure_element = get_reference(measure_element_pointer);
        const std::string measure_element_name = measure_element.Name();
        if (measure_element_name == "attributes") {
          // TODO(brandon): handle attributes mid measure
          const auto *key_pointer = measure_element.FirstChildElement("key");
          if (key_pointer != nullptr) {
            current_midi_key = major_key_to_midi.at(std::stoi(
                get_reference(
                    get_reference(key_pointer).FirstChildElement("fifths"))
                    .GetText()));
          }
          const auto *divisions_pointer =
              measure_element.FirstChildElement("divisions");
          if (divisions_pointer != nullptr) {
            current_divisions =
                std::stoi(get_reference(divisions_pointer).GetText());
          }
        } else if (measure_element_name == "note") {
          const auto note_duration = get_duration(measure_element);

          if (measure_element.FirstChildElement("unpitched") != nullptr) {
            UnpitchedNote new_unpitched_note;
            new_unpitched_note.unpitched_instrument =
                get_instrument(part_instruments, measure_element);
            new_unpitched_note.beats =
                Rational(note_duration, current_divisions);
            if (chord_info_dict.contains(chord_start_time)) {
              auto &existing_chord = chord_info_dict.at(chord_start_time).chord;
              if (existing_chord.unpitched_instrument ==
                  new_unpitched_note.unpitched_instrument) {
                new_unpitched_note.unpitched_instrument = "";
              }
              existing_chord.unpitched_notes.push_back(
                  std::move(new_unpitched_note));
            } else {
              Chord new_chord;
              new_chord.words = std::to_string(measure_number);
              new_chord.unpitched_notes.push_back(
                  std::move(new_unpitched_note));
              new_chord.unpitched_instrument =
                  part_instruments.default_instrument;
              chord_info_dict[chord_start_time] =
                  ChordInfo({std::move(new_chord), current_midi_key,
                              current_divisions});
            }
          } else if (measure_element.FirstChildElement("pitch") != nullptr) {
            bool tie_start = false;
            bool tie_end = false;
            auto note_midi_number = MIDI_C4;

            const auto *tie_pointer =
                measure_element.FirstChildElement("tie");
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

            const auto &pitch =
                get_reference(measure_element.FirstChildElement("pitch"));
            const auto &step = get_reference(pitch.FirstChildElement("step"));
            const auto &octave =
                get_reference(pitch.FirstChildElement("octave"));

            const auto *alterations_pointer =
                pitch.FirstChildElement("alter");
            auto alteration = 0;
            if (alterations_pointer != nullptr) {
              alteration =
                  std::stoi(get_reference(alterations_pointer).GetText());
            }

            note_midi_number =
                note_to_midi.at(step.GetText()) +
                (std::stoi(octave.GetText()) + 1) * HALFSTEPS_PER_OCTAVE +
                alteration;
            if (tie_end) {
              const auto note_info_iterator =
                  tied_pitched_notes_info.find(note_midi_number);
              Q_ASSERT(note_info_iterator != tied_pitched_notes_info.end());
              auto previous_tie_info = note_info_iterator->second;
              auto previous_note = std::move(previous_tie_info.pitched_note);
              previous_note.beats =
                  previous_note.beats +
                  Rational(note_duration, current_divisions);
              if (!tie_start) {
                add_pitched_note_and_maybe_chord(
                    chord_info_dict, note_midi_number,
                    previous_tie_info.start_time,
                    previous_tie_info.measure_number,
                    previous_tie_info.divisions, previous_tie_info.midi_key,
                    std::move(previous_note));
                tied_pitched_notes_info.erase(note_info_iterator);
              }
            } else {
              PitchedNote new_pitched_note;
              new_pitched_note.beats =
                  Rational(note_duration, current_divisions);

              new_pitched_note.pitched_instrument =
                  get_instrument(part_instruments, measure_element);

              if (tie_start) { // also not tie end
                tied_pitched_notes_info[note_midi_number] = PitchedNoteInfo(
                    {std::move(new_pitched_note), measure_number,
                      current_midi_key, chord_start_time});
              } else { // not tie start or end
                add_pitched_note_and_maybe_chord(
                    chord_info_dict, note_midi_number, chord_start_time,
                    measure_number, current_divisions, current_midi_key,
                    std::move(new_pitched_note));
              }
            }
          }
          if (measure_element_pointer->FirstChildElement("chord") ==
              nullptr) {
            if (is_chord) {
              // we've already updated the current time
              // but the chord is over so start a new chord
              chord_start_time = current_time;
              is_chord = false;
            } else {
              current_time = current_time + note_duration;
            }
          } else {
            is_chord = true;
            current_time = current_time + note_duration;
          }
          measure_element_pointer =
              measure_element.NextSiblingElement();
        } else if (measure_element_name == "backup") {
          current_time = current_time - get_duration(measure_element);
          chord_start_time = current_time;
        } else if (measure_element_name == "forward") {
          current_time = current_time + get_duration(measure_element);
          chord_start_time = current_time;
        }
        measure_number = measure_number + 1;
      }
      measure_pointer = measure.NextSiblingElement("measure");
    }
    Q_ASSERT(tied_pitched_notes_info.empty());
    part_pointer = part.NextSiblingElement("part");
  }
  Song song;

  auto &chords = song.chords;

  auto chord_info_iterator = chord_info_dict.begin();
  if (chord_info_iterator == chord_info_dict.end()) {
    Q_ASSERT(false); // TODO(brandon): qt warnnig
    return {};
  }

  int current_time = chord_info_iterator->first;
  auto chord_info = chord_info_iterator->second;
  auto current_midi_key = chord_info.midi_key;
  auto chord = chord_info.chord;
  auto divisions = chord_info.divisions;
  auto current_pitched_instrument = chord.pitched_instrument;
  auto current_unpitched_instrument = chord.unpitched_instrument;
  auto starting_octave =
      (current_midi_key - CONCERT_A_MIDI) / HALFSTEPS_PER_OCTAVE;
  song.starting_key = pow(OCTAVE_RATIO, starting_octave) * CONCERT_A_FREQUENCY;

  chord.interval = Interval(1, 1, 0);

  auto last_midi_key = current_midi_key;
  ++chord_info_iterator;
  while (chord_info_iterator != chord_info_dict.end()) {
    int next_time = chord_info_iterator->first;
    chord.beats = Rational(next_time - current_time, divisions);
    chords.push_back(chord);
    current_midi_key = chord_info.midi_key;
    chord = chord_info.chord;
    divisions = chord_info.divisions;

    auto new_pitched_instrument = chord.pitched_instrument;
    if (current_pitched_instrument == new_pitched_instrument) {
      chord.pitched_instrument = "";
    } else {
      current_pitched_instrument = std::move(new_pitched_instrument);
    }

    auto new_unpitched_instrument = chord.unpitched_instrument;
    if (current_unpitched_instrument == new_unpitched_instrument) {
      chord.unpitched_instrument = "";
    } else {
      current_unpitched_instrument = std::move(new_unpitched_instrument);
    }

    chord.interval = get_interval(current_midi_key - last_midi_key);
    last_midi_key = current_midi_key;
    ++chord_info_iterator;
  }
  auto &last_notes = chord.pitched_notes;
  Q_ASSERT(!last_notes.empty());
  chord.beats = std::max_element(last_notes.begin(), last_notes.end(),
                                 [](const PitchedNote &first_note,
                                    const PitchedNote &second_note) {
                                   return first_note.beats < second_note.beats;
                                 })
                    ->beats;
  chords.push_back(std::move(chord));
  return song;
}

// TODO(brandon): handle instruments better