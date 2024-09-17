#include <algorithm>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>

#include "chord/Chord.hpp"
#include "interval/Interval.hpp"
#include "justly/ChordColumn.hpp"
#include "note/Note.hpp"
#include "other/other.hpp"
#include "percussion/Percussion.hpp"
#include "rational/Rational.hpp"

auto
get_chord_column_schema(const char *description) -> nlohmann::json {
  return nlohmann::json({{"type", "number"},
                         {"description", description},
                         {"minimum", chord_interval_column},
                         {"maximum", chord_percussions_column}});
}

auto get_chords_schema() -> nlohmann::json {
  return nlohmann::json(
      {{"type", "array"},
       {"description", "a list of chords"},
       {"items",
        nlohmann::json(
            {{"type", "object"},
             {"description", "a chord"},
             {"properties",
              nlohmann::json(
                  {{"interval", get_interval_schema()},
                   {"beats", get_rational_schema("the number of beats")},
                   {"velocity_percent", get_rational_schema("velocity ratio")},
                   {"tempo_percent", get_rational_schema("tempo ratio")},
                   {"words", get_words_schema()},
                   {"notes", get_notes_schema()},
                   {"percussions", get_percussions_schema()}})}})}});
}

auto chords_to_json(const QList<Chord> &chords,
                           qsizetype first_chord_number,
                           qsizetype number_of_chords) -> nlohmann::json {
  nlohmann::json json_chords = nlohmann::json::array();
  std::transform(
      chords.cbegin() + static_cast<int>(first_chord_number),
      chords.cbegin() + static_cast<int>(first_chord_number + number_of_chords),
      std::back_inserter(json_chords),
      [](const Chord &chord) -> nlohmann::json {
        auto json_chord = nlohmann::json::object();
        const auto &interval = chord.interval;
        if (!interval_is_default(interval)) {
          json_chord["interval"] = interval_to_json(interval);
        }

        const auto &beats = chord.beats;
        if (!(rational_is_default(beats))) {
          json_chord["beats"] = rational_to_json(beats);
        }
        const auto &velocity_ratio = chord.velocity_ratio;
        if (!(rational_is_default(velocity_ratio))) {
          json_chord["velocity_ratio"] = rational_to_json(velocity_ratio);
        }
        const auto &tempo_ratio = chord.tempo_ratio;
        if (!(rational_is_default(tempo_ratio))) {
          json_chord["tempo_ratio"] = rational_to_json(tempo_ratio);
        }
        const auto &words = chord.words;
        if (!words.isEmpty()) {
          json_chord["words"] = words.toStdString().c_str();
        }
        const auto &notes = chord.notes;
        if (!notes.empty()) {
          json_chord["notes"] = notes_to_json(notes, 0, notes.size());
        }
        const auto &percussions = chord.percussions;
        if (!percussions.empty()) {
          json_chord["percussions"] =
              percussions_to_json(percussions, 0, percussions.size());
        }
        return json_chord;
      });
  return json_chords;
}

void json_to_chords(QList<Chord> &new_chords,
                           const nlohmann::json &json_chords,
                           qsizetype number_of_chords) {
  std::transform(
      json_chords.cbegin(),
      json_chords.cbegin() + static_cast<int>(number_of_chords),
      std::back_inserter(new_chords),
      [](const nlohmann::json &json_chord) -> Chord {
        Chord chord;
        if (json_chord.contains("interval")) {
          chord.interval = json_to_interval(json_chord["interval"]);
        }
        if (json_chord.contains("beats")) {
          chord.beats = json_to_rational(json_chord["beats"]);
        }
        if (json_chord.contains("velocity_ratio")) {
          chord.velocity_ratio = json_to_rational(json_chord["velocity_ratio"]);
        }
        if (json_chord.contains("tempo_ratio")) {
          chord.tempo_ratio = json_to_rational(json_chord["tempo_ratio"]);
        }
        if (json_chord.contains("words")) {
          chord.words = QString::fromStdString(json_chord.value("words", ""));
        }
        if (json_chord.contains("notes")) {
          const auto &json_notes = json_chord["notes"];
          json_to_notes(chord.notes, json_notes, json_notes.size());
        }
        if (json_chord.contains("percussions")) {
          const auto &json_percussions = json_chord["percussions"];
          json_to_percussions(chord.percussions, json_percussions,
                              json_percussions.size());
        }
        return chord;
      });
}


