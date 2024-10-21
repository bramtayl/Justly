#include "chord/Chord.hpp"

#include <algorithm>
#include <iterator>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <string>

#include "interval/Interval.hpp"
#include "justly/ChordColumn.hpp"
#include "note/Note.hpp"
#include "other/other.hpp"
#include "percussion/Percussion.hpp"
#include "rational/Rational.hpp"

[[nodiscard]] static auto
get_chord_column_schema(const char *description) -> nlohmann::json {
  return nlohmann::json({{"type", "number"},
                         {"description", description},
                         {"minimum", chord_instrument_column},
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
                  {{"instrument", get_instrument_schema()},
                   {"percussion_set", get_percussion_set_schema()},
                   {"percussion_instrument",
                    get_percussion_instrument_schema()},
                   {"interval", get_interval_schema()},
                   {"beats", get_rational_schema("the number of beats")},
                   {"velocity_percent", get_rational_schema("velocity ratio")},
                   {"tempo_percent", get_rational_schema("tempo ratio")},
                   {"words", get_words_schema()},
                   {"notes", get_notes_schema()},
                   {"percussions", get_percussions_schema()}})}})}});
}

auto get_chords_cells_validator()
    -> const nlohmann::json_schema::json_validator & {
  static const nlohmann::json_schema::json_validator chords_cells_validator =
      make_validator(
          "Chords cells",
          nlohmann::json(
              {{"description", "chords cells"},
               {"type", "object"},
               {"required", {"left_column", "right_column", "chords"}},
               {"properties",
                nlohmann::json({{"left_column",
                                 get_chord_column_schema("left ChordColumn")},
                                {"right_column",
                                 get_chord_column_schema("right ChordColumn")},
                                {"chords", get_chords_schema()}})}}));
  return chords_cells_validator;
}

auto chords_to_json(const QList<Chord> &chords, qsizetype first_chord_number,
                    qsizetype number_of_chords, ChordColumn left_column,
                    ChordColumn right_column) -> nlohmann::json {
  nlohmann::json json_chords = nlohmann::json::array();
  std::transform(
      chords.cbegin() + static_cast<int>(first_chord_number),
      chords.cbegin() + static_cast<int>(first_chord_number + number_of_chords),
      std::back_inserter(json_chords),
      [left_column, right_column](const Chord &chord) -> nlohmann::json {
        auto json_chord = nlohmann::json::object();

        const auto *instrument_pointer = chord.instrument_pointer;
        const auto *percussion_set_pointer = chord.percussion_set_pointer;
        const auto *percussion_instrument_pointer =
            chord.percussion_instrument_pointer;
        const auto &interval = chord.interval;
        const auto &beats = chord.beats;
        const auto &tempo_ratio = chord.tempo_ratio;
        const auto &velocity_ratio = chord.velocity_ratio;
        const auto &words = chord.words;
        const auto &notes = chord.notes;
        const auto &percussions = chord.percussions;

        for (auto chord_column = left_column; chord_column <= right_column;
             chord_column = static_cast<ChordColumn>(chord_column + 1)) {
          switch (chord_column) {
          case chord_instrument_column:
            if (!instrument_pointer_is_default(instrument_pointer)) {
              json_chord["instrument"] =
                  instrument_pointer_to_json(instrument_pointer);
            }
            break;
          case chord_percussion_set_column:
            if (!percussion_set_pointer_is_default(percussion_set_pointer)) {
              json_chord["percussion_set"] =
                  percussion_set_pointer_to_json(percussion_set_pointer);
            }
            break;
          case chord_percussion_instrument_column:
            if (!percussion_instrument_pointer_is_default(
                    percussion_instrument_pointer)) {
              json_chord["percussion_instrument"] =
                  percussion_instrument_pointer_to_json(
                      percussion_instrument_pointer);
            }
            break;
          case chord_interval_column:
            if (!interval_is_default(interval)) {
              json_chord["interval"] = interval_to_json(interval);
            }
            break;
          case chord_beats_column:
            if (!(rational_is_default(beats))) {
              json_chord["beats"] = rational_to_json(beats);
            }
            break;
          case chord_velocity_ratio_column:
            if (!(rational_is_default(velocity_ratio))) {
              json_chord["velocity_ratio"] = rational_to_json(velocity_ratio);
            }
            break;
          case chord_tempo_ratio_column:
            if (!(rational_is_default(tempo_ratio))) {
              json_chord["tempo_ratio"] = rational_to_json(tempo_ratio);
            }
            break;
          case chord_words_column:
            if (!words.isEmpty()) {
              json_chord["words"] = words.toStdString().c_str();
            }
            break;
          case chord_notes_column:
            if (!notes.empty()) {
              json_chord["notes"] = notes_to_json(notes, 0, notes.size());
            }
            break;
          case chord_percussions_column:
            if (!percussions.empty()) {
              json_chord["percussions"] =
                  percussions_to_json(percussions, 0, percussions.size());
            }
            break;
          }
        }
        return json_chord;
      });
  return json_chords;
}

void partial_json_to_chords(QList<Chord> &new_chords,
                            const nlohmann::json &json_chords,
                            size_t number_of_chords) {
  std::transform(
      json_chords.cbegin(),
      json_chords.cbegin() + static_cast<int>(number_of_chords),
      std::back_inserter(new_chords),
      [](const nlohmann::json &json_chord) -> Chord {
        Chord chord;
        if (json_chord.contains("instrument")) {
          chord.instrument_pointer =
              json_to_instrument_pointer(json_chord["instrument"]);
        };
        if (json_chord.contains("percussion_set")) {
          chord.percussion_set_pointer =
              json_to_percussion_set_pointer(json_chord["percussion_set"]);
        }
        if (json_chord.contains("percussion_instrument")) {
          chord.percussion_instrument_pointer =
              json_to_percussion_instrument_pointer(
                  json_chord["percussion_instrument"]);
        }
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
          json_to_notes(chord.notes, json_notes);
        }
        if (json_chord.contains("percussions")) {
          const auto &json_percussions = json_chord["percussions"];
          json_to_percussions(chord.percussions, json_percussions);
        }
        return chord;
      });
}

void json_to_chords(QList<Chord> &new_chords,
                    const nlohmann::json &json_chords) {
  partial_json_to_chords(new_chords, json_chords, json_chords.size());
}
