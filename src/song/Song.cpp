#include "justly/Song.hpp"

#include <qassert.h>  // for Q_ASSERT

#include <algorithm>                         // for transform
#include <cstddef>                           // for size_t
#include <map>                               // for operator!=, operator==
#include <nlohmann/detail/json_pointer.hpp>  // for json_pointer<>::string_t
#include <nlohmann/json.hpp>                 // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>             // for json
#include <string>                            // for string

#include "json/json.hpp"          // for from_json, to_json
#include "justly/Chord.hpp"       // for Chord
#include "justly/Instrument.hpp"  // for get_instrument_pointer

const auto DEFAULT_STARTING_KEY = 220;
const auto DEFAULT_STARTING_VOLUME = 50;
const auto DEFAULT_STARTING_TEMPO = 100;
const auto DEFAULT_STARTING_INSTRUMENT = "Marimba";

Song::Song()
    : starting_key(DEFAULT_STARTING_KEY),
      starting_volume(DEFAULT_STARTING_VOLUME),
      starting_tempo(DEFAULT_STARTING_TEMPO),
      starting_instrument_pointer(
          get_instrument_pointer(DEFAULT_STARTING_INSTRUMENT)) {}

auto Song::get_number_of_children(int parent_number) const -> size_t {
  if (parent_number == -1) {
    return chord_pointers.size();
  }

  Q_ASSERT(0 <= parent_number);
  Q_ASSERT(static_cast<size_t>(parent_number) < chord_pointers.size());
  const auto& chord_pointer = chord_pointers[parent_number];
  Q_ASSERT(chord_pointer != nullptr);

  return chord_pointer->note_pointers.size();
};

auto Song::json() const -> nlohmann::json {
  nlohmann::json json_song;
  json_song["starting_key"] = starting_key;
  json_song["starting_tempo"] = starting_tempo;
  json_song["starting_volume"] = starting_volume;
  Q_ASSERT(starting_instrument_pointer != nullptr);
  json_song["starting_instrument"] =
      starting_instrument_pointer->instrument_name;
  json_song["chords"] = to_json(chord_pointers, 0, chord_pointers.size());
  return json_song;
}

void Song::load_starting_values(const nlohmann::json& json_song) {
  Q_ASSERT(json_song.contains("starting_key"));
  const auto& starting_key_value = json_song["starting_key"];
  Q_ASSERT(starting_key_value.is_number());
  starting_key = starting_key_value.get<double>();

  Q_ASSERT(json_song.contains("starting_volume"));
  const auto& starting_volume_value = json_song["starting_volume"];
  Q_ASSERT(starting_volume_value.is_number());
  starting_volume = starting_volume_value.get<double>();

  Q_ASSERT(json_song.contains("starting_tempo"));
  const auto& starting_tempo_value = json_song["starting_tempo"];
  Q_ASSERT(starting_tempo_value.is_number());
  starting_tempo = starting_tempo_value.get<double>();

  Q_ASSERT(json_song.contains("starting_instrument"));
  const auto& starting_instrument_value = json_song["starting_instrument"];
  Q_ASSERT(starting_instrument_value.is_string());
  starting_instrument_pointer =
      get_instrument_pointer(starting_instrument_value.get<std::string>());
}

void Song::load_chords(const nlohmann::json& json_song) {
  chord_pointers.clear();
  if (json_song.contains("chords")) {
    from_json(&chord_pointers, 0, json_song["chords"]);
  }
}
