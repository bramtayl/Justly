#include "chord/Chord.hpp"

#include <QtGlobal>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

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
#include "rows/json_field_conversions.hpp"

auto to_chord_column(int column) -> ChordColumn {
  Q_ASSERT(column >= 0);
  Q_ASSERT(column < NUMBER_OF_CHORD_COLUMNS);
  return static_cast<ChordColumn>(column);
}

void Chord::from_json(const nlohmann::json &json_chord) {
  instrument_from_json(*this, json_chord);
  percussion_set_pointer_from_json(*this, json_chord);
  percussion_instrument_pointer_from_json(*this, json_chord);
  interval_from_json(*this, json_chord);
  beats_from_json(*this, json_chord);
  velocity_ratio_from_json(*this, json_chord);
  if (json_chord.contains("tempo_ratio")) {
    tempo_ratio = json_to_rational(json_chord["tempo_ratio"]);
  }
  words_from_json(*this, json_chord);
  if (json_chord.contains("notes")) {
    const auto &json_notes = json_chord["notes"];
    json_to_rows(notes, json_notes);
  }
  if (json_chord.contains("percussions")) {
    const auto &json_percussions = json_chord["percussions"];
    json_to_rows(percussions, json_percussions);
  }
}

auto Chord::get_data(int column_number) const -> QVariant {
  switch (to_chord_column(column_number)) {
  case chord_instrument_column:
    return QVariant::fromValue(instrument_pointer);
  case chord_percussion_set_column:
    return QVariant::fromValue(percussion_set_pointer);
  case chord_percussion_instrument_column:
    return QVariant::fromValue(percussion_instrument_pointer);
  case chord_interval_column:
    return QVariant::fromValue(interval);
  case chord_beats_column:
    return QVariant::fromValue(beats);
  case chord_velocity_ratio_column:
    return QVariant::fromValue(velocity_ratio);
  case chord_tempo_ratio_column:
    return QVariant::fromValue(tempo_ratio);
  case chord_words_column:
    return words;
  case chord_notes_column:
    return notes.size();
  case chord_percussions_column:
    return percussions.size();
  }
}

void Chord::set_data_directly(int column, const QVariant &new_value) {
  switch (to_chord_column(column)) {
  case chord_instrument_column:
    instrument_pointer = variant_to_instrument(new_value);
    break;
  case chord_percussion_set_column:
    percussion_set_pointer = variant_to_percussion_set(new_value);
    break;
  case chord_percussion_instrument_column:
    percussion_instrument_pointer = variant_to_percussion_instrument(new_value);
    break;
  case chord_interval_column:
    interval = variant_to_interval(new_value);
    break;
  case chord_beats_column:
    beats = variant_to_rational(new_value);
    break;
  case chord_velocity_ratio_column:
    velocity_ratio = variant_to_rational(new_value);
    break;
  case chord_tempo_ratio_column:
    tempo_ratio = variant_to_rational(new_value);
    break;
  case chord_words_column:
    words = variant_to_string(new_value);
    break;
  default:
    Q_ASSERT(false);
    break;
  }
};

void Chord::copy_columns_from(const Chord &template_row, int left_column,
                              int right_column) {
  for (auto chord_column = left_column; chord_column <= right_column;
       chord_column++) {
    switch (to_chord_column(chord_column)) {
    case chord_instrument_column:
      instrument_pointer = template_row.instrument_pointer;
      break;
    case chord_percussion_set_column:
      percussion_set_pointer = template_row.percussion_set_pointer;
      break;
    case chord_percussion_instrument_column:
      percussion_instrument_pointer =
          template_row.percussion_instrument_pointer;
      break;
    case chord_interval_column:
      interval = template_row.interval;
      break;
    case chord_beats_column:
      beats = template_row.beats;
      break;
    case chord_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    case chord_tempo_ratio_column:
      tempo_ratio = template_row.tempo_ratio;
      break;
    case chord_words_column:
      words = template_row.words;
      break;
    case chord_notes_column:
      notes = template_row.notes;
      break;
    case chord_percussions_column:
      percussions = template_row.percussions;
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
      add_named_to_json(json_chord, instrument_pointer, "instrument");
      break;
    case chord_percussion_set_column:
      add_named_to_json(json_chord, percussion_set_pointer, "percussion_set");
      break;
    case chord_percussion_instrument_column:
      add_named_to_json(json_chord, percussion_instrument_pointer,
                        "percussion_instrument");
    case chord_interval_column:
      add_interval_to_json(json_chord, interval);
      break;
    case chord_beats_column:
      add_rational_to_json(json_chord, beats, "beats");
      break;
    case chord_velocity_ratio_column:
      add_rational_to_json(json_chord, velocity_ratio, "velocity_ratio");
      break;
    case chord_tempo_ratio_column:
      add_rational_to_json(json_chord, tempo_ratio, "tempo_ratio");
      break;
    case chord_words_column:
      add_words_to_json(json_chord, words);
      break;
    case chord_notes_column:
      if (!notes.empty()) {
        json_chord["notes"] =
            rows_to_json(notes, 0, static_cast<int>(notes.size()),
                         note_instrument_column, note_words_column);
      }
      break;
    case chord_percussions_column:
      if (!percussions.empty()) {
        json_chord["percussions"] = rows_to_json(
            percussions, 0, static_cast<int>(percussions.size()),
            percussion_percussion_set_column, percussion_velocity_ratio_column);
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
