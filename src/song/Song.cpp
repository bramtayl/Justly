#include "justly/Song.hpp"

#include <algorithm>                         // for transform
#include <cstddef>                           // for size_t
#include <map>                               // for operator!=, operator==
#include <nlohmann/detail/json_pointer.hpp>  // for json_pointer<>::string_t
#include <nlohmann/json.hpp>                 // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>             // for json
#include <string>                            // for string

#include "justly/Chord.hpp"       // for Chord
#include "justly/Instrument.hpp"  // for get_instrument, Instrument
#include "song/json.hpp"           // for from_json, objec...

const auto DEFAULT_STARTING_KEY = 220;
const auto DEFAULT_STARTING_VOLUME = 50;
const auto DEFAULT_STARTING_TEMPO = 100;
const auto DEFAULT_STARTING_INSTRUMENT = "Marimba";

Song::Song()
    : starting_key(DEFAULT_STARTING_KEY),
      starting_volume(DEFAULT_STARTING_VOLUME),
      starting_tempo(DEFAULT_STARTING_TEMPO),
      starting_instrument_pointer(
          &(get_instrument(DEFAULT_STARTING_INSTRUMENT))) {}

auto Song::get_number_of_children(int parent_number) const -> size_t {
  if (parent_number == -1) {
    return chord_pointers.size();
  }
  return chord_pointers[parent_number]->note_pointers.size();
};

auto Song::json() const -> nlohmann::json {
  nlohmann::json json_song;
  json_song["starting_key"] = starting_key;
  json_song["starting_tempo"] = starting_tempo;
  json_song["starting_volume"] = starting_volume;
  json_song["starting_instrument"] =
      starting_instrument_pointer->instrument_name;
  json_song["chords"] = to_json(
      chord_pointers, 0, chord_pointers.size());
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
    from_json(&chord_pointers, 0, json_song["chords"]);
  }
}
