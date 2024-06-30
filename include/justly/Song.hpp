#pragma once

#include <cstddef>                // for size_t
#include <nlohmann/json_fwd.hpp>  // for json
#include <vector>                 // for vector

#include "justly/public_constants.hpp"
#include "justly/Chord.hpp"  // for Chord

struct Instrument;

struct JUSTLY_EXPORT Song {
  double starting_key;
  double starting_volume;
  double starting_tempo;
  const Instrument *starting_instrument_pointer;
  std::vector<Chord> chords;

  Song();

  [[nodiscard]] auto get_number_of_children(int parent_number) const -> size_t;
  auto get_chord_number(Chord *chord_pointer) const -> int;

  [[nodiscard]] auto json() const -> nlohmann::json;
  void load_starting_values(const nlohmann::json & json_song);
  void load_chords(const nlohmann::json & json_song);
};
