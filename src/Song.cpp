#include "justly/Song.h"

#include <algorithm>                         // for transform
#include <cstddef>                           // for size_t
#include <map>                               // for operator!=, operator==
#include <nlohmann/detail/json_pointer.hpp>  // for json_pointer<>::string_t
#include <nlohmann/json.hpp>                 // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>             // for json
#include <string>                            // for string

#include "justly/Chord.h"       // for Chord
#include "justly/Instrument.h"  // for get_instrument, Instrument
#include "src/json.h"           // for objects_from_json, objec...

const auto DEFAULT_STARTING_KEY = 220;
const auto DEFAULT_STARTING_VOLUME = 90;
const auto DEFAULT_STARTING_TEMPO = 200;
const auto DEFAULT_STARTING_INSTRUMENT = "Marimba";

Song::Song()
    : starting_key(DEFAULT_STARTING_KEY),
      starting_volume(DEFAULT_STARTING_VOLUME),
      starting_tempo(DEFAULT_STARTING_TEMPO),
      starting_instrument_pointer(
          &(get_instrument(DEFAULT_STARTING_INSTRUMENT))) {}

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
  json_song["chords"] = objects_to_json(
      chord_pointers, 0, static_cast<int>(chord_pointers.size()));
  return json_song;
}

void Song::load_starting_values(const nlohmann::json& json_song) {
  starting_key = json_song["starting_key"].get<double>();
  starting_volume = json_song["starting_volume"].get<double>();
  starting_tempo = json_song["starting_tempo"].get<double>();
  starting_instrument_pointer =
      &(get_instrument(json_song["starting_instrument"].get<std::string>()));
}

void Song::load_chords(const nlohmann::json& json_song) {
  chord_pointers.clear();
  if (json_song.contains("chords")) {
    objects_from_json(&chord_pointers, 0, json_song["chords"]);
  }
}

auto Song::get_number_of_children(int parent_number) const -> int {
  if (parent_number == -1) {
    return static_cast<int>(chord_pointers.size());
  }
  return static_cast<int>(
      chord_pointers[parent_number]->note_pointers.size());
};
