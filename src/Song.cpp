#include "justly/Song.h"

#include <algorithm>                         // for max
#include <map>                               // for operator!=
#include <nlohmann/detail/json_pointer.hpp>  // for json_pointer<>::string_t
#include <nlohmann/detail/json_ref.hpp>      // for json_ref
#include <nlohmann/json-schema.hpp>          // for json_validator
#include <nlohmann/json.hpp>                 // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>             // for json
#include <string>                            // for string

#include "justly/Chord.h"          // for Chord
#include "justly/Instrument.h"        // for Instrument
#include "src/JsonErrorHandler.h"  // for JsonErrorHandler

const auto DEFAULT_STARTING_KEY = 220;
const auto DEFAULT_STARTING_VOLUME = 90;
const auto DEFAULT_STARTING_TEMPO = 200;
const auto DEFAULT_STARTING_INSTRUMENT = "Marimba";

Song::Song()
    : starting_key(DEFAULT_STARTING_KEY),
      starting_volume(DEFAULT_STARTING_VOLUME),
      starting_tempo(DEFAULT_STARTING_TEMPO),
      starting_instrument_pointer(
          &(Instrument::get_instrument(DEFAULT_STARTING_INSTRUMENT))) {}

auto Song::json() const -> nlohmann::json {
  nlohmann::json json_song;
  json_song["$schema"] =
      "https://raw.githubusercontent.com/bramtayl/Justly/"
      "master/song_schema.json";
  json_song["starting_key"] = starting_key;
  json_song["starting_tempo"] = starting_tempo;
  json_song["starting_volume"] = starting_volume;
  json_song["starting_instrument"] =
      starting_instrument_pointer->instrument_name;
  json_song["chords"] = to_json(
      chord_pointers, 0, static_cast<int>(chord_pointers.size()));
  return json_song;
}

auto Song::verify_json(const nlohmann::json& json_song) -> bool {
  JsonErrorHandler error_handler;

  static const nlohmann::json_schema::json_validator validator(
      nlohmann::json({{"$schema", "http://json-schema.org/draft-07/schema#"},
                      {"title", "Song"},
                      {"description", "A Justly song in JSON format"},
                      {"type", "object"},
                      {"required",
                       {"starting_key", "starting_tempo", "starting_volume",
                        "starting_instrument"}},
                      {"properties",
                       {{"starting_instrument",
                         {{"type", "string"},
                          {"description", "the starting instrument"},
                          {"enum", Instrument::instrument_names()}}},
                        {"starting_key",
                         {{"type", "integer"},
                          {"description", "the starting key, in Hz"},
                          {"minimum", MIN_STARTING_KEY},
                          {"maximum", MAX_STARTING_KEY}}},
                        {"starting_tempo",
                         {{"type", "integer"},
                          {"description", "the starting tempo, in bpm"},
                          {"minimum", MIN_STARTING_TEMPO},
                          {"maximum", MAX_STARTING_TEMPO}}},
                        {"starting_volume",
                         {{"type", "integer"},
                          {"description", "the starting volume, from 1 to 100"},
                          {"minimum", MIN_STARTING_VOLUME},
                          {"maximum", MAX_STARTING_VOLUME}}},
                        {"chords",
                         {{"type", "array"},
                          {"description", "a list of chords"},
                          {"items", Chord::json_schema()}}}}}}));

  validator.validate(json_song, error_handler);
  return !error_handler;
}

void Song::load(const nlohmann::json& json_song) {
  starting_key = json_song["starting_key"].get<double>();
  starting_volume = json_song["starting_volume"].get<double>();
  starting_tempo = json_song["starting_tempo"].get<double>();
  starting_instrument_pointer = &(Instrument::get_instrument(
      json_song["starting_instrument"].get<std::string>()));
  chord_pointers.clear();
  if (json_song.contains("chords")) {
    from_json(&chord_pointers, 0, json_song["chords"]);
  }
}
