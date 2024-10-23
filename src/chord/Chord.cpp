#include "chord/Chord.hpp"

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <string>

#include "chord/ChordsModel.hpp"
#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "justly/ChordColumn.hpp"
#include "justly/NoteColumn.hpp"
#include "justly/PercussionColumn.hpp"
#include "note/Note.hpp"
#include "other/other.hpp"
#include "percussion/Percussion.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "rational/Rational.hpp"

Chord::Chord(const nlohmann::json &json_chord) {
  if (json_chord.contains("instrument")) {
    instrument_pointer =
        &json_to_item(get_all_instruments(), json_chord["instrument"]);
  };
  if (json_chord.contains("percussion_set")) {
    percussion_set_pointer =
        &json_to_item(get_all_percussion_sets(), json_chord["percussion_set"]);
  }
  if (json_chord.contains("percussion_instrument")) {
    percussion_instrument_pointer = &json_to_item(
        get_all_percussion_instruments(), json_chord["percussion_instrument"]);
  }
  if (json_chord.contains("interval")) {
    interval = json_to_interval(json_chord["interval"]);
  }
  if (json_chord.contains("beats")) {
    beats = json_to_rational(json_chord["beats"]);
  }
  if (json_chord.contains("velocity_ratio")) {
    velocity_ratio = json_to_rational(json_chord["velocity_ratio"]);
  }
  if (json_chord.contains("tempo_ratio")) {
    tempo_ratio = json_to_rational(json_chord["tempo_ratio"]);
  }
  if (json_chord.contains("words")) {
    words = QString::fromStdString(json_chord.value("words", ""));
  }
  if (json_chord.contains("notes")) {
    const auto &json_notes = json_chord["notes"];
    json_to_rows(notes, json_notes);
  }
  if (json_chord.contains("percussions")) {
    const auto &json_percussions = json_chord["percussions"];
    json_to_rows(percussions, json_percussions);
  }
}

void Chord::copy_columns_from(const Chord& template_chord, int left_column, int right_column) {
  for (auto chord_column = left_column; chord_column <= right_column;
        chord_column++) {
    switch (to_chord_column(chord_column)) {
    case chord_instrument_column:
      instrument_pointer = template_chord.instrument_pointer;
      break;
    case chord_percussion_set_column:
      percussion_set_pointer = template_chord.percussion_set_pointer;
      break;
    case chord_percussion_instrument_column:
      percussion_instrument_pointer =
          template_chord.percussion_instrument_pointer;
      break;
    case chord_interval_column:
      interval = template_chord.interval;
      break;
    case chord_beats_column:
      beats = template_chord.beats;
      break;
    case chord_velocity_ratio_column:
      velocity_ratio = template_chord.velocity_ratio;
      break;
    case chord_tempo_ratio_column:
      tempo_ratio = template_chord.tempo_ratio;
      break;
    case chord_words_column:
      words = template_chord.words;
      break;
    case chord_notes_column:
      notes = template_chord.notes;
      break;
    case chord_percussions_column:
      percussions = template_chord.percussions;
      break;
    }
  }
};

[[nodiscard]] auto Chord::to_json(int left_column,
                                  int right_column) const -> nlohmann::json {
  auto json_chord = nlohmann::json::object();

  for (auto chord_column = left_column; chord_column <= right_column;
       chord_column = chord_column + 1) {
    switch (to_chord_column(chord_column)) {
    case chord_instrument_column:
      if (instrument_pointer != nullptr) {
        json_chord["instrument"] = item_to_json(*instrument_pointer);
      }
      break;
    case chord_percussion_set_column:
      if (percussion_set_pointer != nullptr) {
        json_chord["percussion_set"] = item_to_json(*percussion_set_pointer);
      }
      break;
    case chord_percussion_instrument_column:
      if (percussion_instrument_pointer != nullptr) {
        json_chord["percussion_instrument"] =
            item_to_json(*percussion_instrument_pointer);
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
        json_chord["notes"] =
            rows_to_json(notes, 0, static_cast<int>(notes.size()), note_instrument_column, note_words_column);
      }
      break;
    case chord_percussions_column:
      if (!percussions.empty()) {
        json_chord["percussions"] =
            rows_to_json(percussions, 0, static_cast<int>(percussions.size()), percussion_set_column, percussion_tempo_ratio_column);
      }
      break;
    }
  }
  return json_chord;
}

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
