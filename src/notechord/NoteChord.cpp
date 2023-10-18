#include "notechord/NoteChord.h"

#include <gsl/pointers>  // for not_null
#include <map>           // for operator!=, operator==
#include <nlohmann/detail/json_pointer.hpp>  // for json_pointer<>::string_t
#include <nlohmann/detail/json_ref.hpp>      // for json_ref
#include <nlohmann/json.hpp>                 // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>             // for json
#include <string>                            // for string

#include "metatypes/Instrument.h"  // for Instrument
#include "metatypes/Interval.h"    // for Interval

NoteChord::NoteChord(const nlohmann::json& json_note_chord)
    : interval(json_note_chord.contains("interval")
                   ? Interval(json_note_chord["interval"])
                   : Interval()),
      beats(json_note_chord.value("beats", DEFAULT_BEATS)),
      volume_percent(
          json_note_chord.value("volume_percent", DEFAULT_VOLUME_PERCENT)),
      tempo_percent(
          json_note_chord.value("tempo_percent", DEFAULT_TEMPO_PERCENT)),
      words(json_note_chord.value("words", DEFAULT_WORDS)),
      instrument_pointer(&Instrument::get_instrument_by_name(
          json_note_chord.value("instrument", ""))) {}

auto NoteChord::to_json() const -> nlohmann::json {
  auto json_note_chord = nlohmann::json::object();
  if (!(interval.is_default())) {
    json_note_chord["interval"] = interval.to_json();
  }
  if (beats != DEFAULT_BEATS) {
    json_note_chord["beats"] = beats;
  }
  if (volume_percent != DEFAULT_VOLUME_PERCENT) {
    json_note_chord["volume_percent"] = volume_percent;
  }
  if (tempo_percent != DEFAULT_TEMPO_PERCENT) {
    json_note_chord["tempo_percent"] = tempo_percent;
  }
  if (words != DEFAULT_WORDS) {
    json_note_chord["words"] = words;
  }
  const auto& instrument_name = instrument_pointer->instrument_name;
  if (!instrument_name.empty()) {
    json_note_chord["instrument"] = instrument_name;
  }
  return json_note_chord;
}

auto NoteChord::get_instrument_schema() -> nlohmann::json& {
  static nlohmann::json instrument_schema(
      {{"type", "string"},
       {"description", "the instrument"},
       {"enum", Instrument::get_all_instrument_names()}});
  return instrument_schema;
}

auto NoteChord::get_beats_schema() -> nlohmann::json& {
  static nlohmann::json instrument_schema(
      {{"type", "integer"},
       {"description", "the number of beats"},
       {"minimum", MINIMUM_BEATS},
       {"maximum", MAXIMUM_BEATS}});
  return instrument_schema;
}

auto NoteChord::get_words_schema() -> nlohmann::json& {
  static nlohmann::json words_schema(
      {{"type", "string"}, {"description", "the words"}});
  return words_schema;
}

auto NoteChord::get_volume_percent_schema() -> nlohmann::json& {
  static nlohmann::json volume_percent_schema(
      {{"type", "number"},
       {"description", "the volume percent"},
       {"minimum", MINIMUM_VOLUME_PERCENT},
       {"maximum", MAXIMUM_VOLUME_PERCENT}});
  return volume_percent_schema;
}

auto NoteChord::get_tempo_percent_schema() -> nlohmann::json& {
  static nlohmann::json tempo_percent_schema(
      {{"type", "number"},
       {"description", "the tempo percent"},
       {"minimum", MINIMUM_TEMPO_PERCENT},
       {"maximum", MAXIMUM_TEMPO_PERCENT}});
  return tempo_percent_schema;
}

