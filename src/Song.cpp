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
#include "src/Instrument.h"     // for Instrument
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
          &(Instrument::get_instrument_by_name(DEFAULT_STARTING_INSTRUMENT))) {}

auto Song::to_json() const -> nlohmann::json {
  nlohmann::json json_song;
  json_song["$schema"] =
      "https://raw.githubusercontent.com/bramtayl/Justly/"
      "master/song_schema.json";
  json_song["starting_key"] = starting_key;
  json_song["starting_tempo"] = starting_tempo;
  json_song["starting_volume"] = starting_volume;
  json_song["starting_instrument"] =
      starting_instrument_pointer->instrument_name;
  json_song["chords"] =
      children_to_json(0, static_cast<int>(chord_pointers.size()));
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
                          {"enum", Instrument::get_all_instrument_names()}}},
                        {"starting_key",
                         {{"type", "integer"},
                          {"description", "the starting key, in Hz"},
                          {"minimum", MINIMUM_STARTING_KEY},
                          {"maximum", MAXIMUM_STARTING_KEY}}},
                        {"starting_tempo",
                         {{"type", "integer"},
                          {"description", "the starting tempo, in bpm"},
                          {"minimum", MINIMUM_STARTING_TEMPO},
                          {"maximum", MAXIMUM_STARTING_TEMPO}}},
                        {"starting_volume",
                         {{"type", "integer"},
                          {"description", "the starting volume, from 1 to 100"},
                          {"minimum", MINIMUM_STARTING_VOLUME},
                          {"maximum", MAXIMUM_STARTING_VOLUME}}},
                        {"chords",
                         {{"type", "array"},
                          {"description", "a list of chords"},
                          {"items", Chord::get_schema()}}}}}}));

  validator.validate(json_song, error_handler);
  return !error_handler;
}

void Song::load_from(const nlohmann::json& json_song) {
  starting_key = json_song["starting_key"].get<double>();
  starting_volume = json_song["starting_volume"].get<double>();
  starting_tempo = json_song["starting_tempo"].get<double>();
  starting_instrument_pointer = &(Instrument::get_instrument_by_name(
      json_song["starting_instrument"].get<std::string>()));
  remove_children(0, static_cast<int>(chord_pointers.size()));
  if (json_song.contains("chords")) {
    insert_json_chilren(0, json_song["chords"]);
  }
}

auto Song::children_to_json(int first_chord_number, int number_of_chords) const
    -> nlohmann::json {
  nlohmann::json json_children;
  for (int chord_number = first_chord_number;
       chord_number < first_chord_number + number_of_chords;
       chord_number = chord_number + 1) {
    json_children.push_back(chord_pointers[chord_number]->to_json());
  }
  return json_children;
}

void Song::insert_empty_chilren(int first_chord_number, int number_of_chords) {
  for (int chord_number = first_chord_number;
       chord_number < first_chord_number + number_of_chords;
       chord_number = chord_number + 1) {
    // will error if childless
    chord_pointers.insert(chord_pointers.begin() + chord_number,
                          std::make_unique<Chord>());
  }
}

void Song::remove_children(int first_chord_number, int number_of_chords) {
  chord_pointers.erase(
      chord_pointers.begin() + first_chord_number,
      chord_pointers.begin() + first_chord_number + number_of_chords);
}

void Song::insert_json_chilren(int first_chord_number,
                              const nlohmann::json& json_children) {
  for (int insertion_number = 0;
       insertion_number < static_cast<int>(json_children.size());
       insertion_number = insertion_number + 1) {
    chord_pointers.insert(
        chord_pointers.begin() + first_chord_number + insertion_number,
        std::make_unique<Chord>(json_children[insertion_number]));
  }
}
