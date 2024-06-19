#pragma once

#include <cstddef>                // for size_t
#include <memory>                 // for unique_ptr
#include <nlohmann/json_fwd.hpp>  // for json
#include <vector>                 // for vector

#include "justly/global.h"
#include "justly/Chord.h"  // for Chord

struct Instrument;

JUSTLY_EXPORT const auto MIN_STARTING_KEY = 60;
JUSTLY_EXPORT const auto MAX_STARTING_KEY = 440;

JUSTLY_EXPORT const auto MIN_STARTING_VOLUME = 1;
JUSTLY_EXPORT const auto MAX_STARTING_VOLUME = 100;

JUSTLY_EXPORT const auto MIN_STARTING_TEMPO = 100;
JUSTLY_EXPORT const auto MAX_STARTING_TEMPO = 800;

struct JUSTLY_EXPORT Song {
  double starting_key;
  double starting_volume;
  double starting_tempo;
  const Instrument *starting_instrument_pointer;
  std::vector<std::unique_ptr<Chord>> chord_pointers;

  Song();

  [[nodiscard]] auto json() const -> nlohmann::json;

  [[nodiscard]] auto get_number_of_children(int) const -> size_t;
  void load_starting_values(const nlohmann::json &);
  void load_chords(const nlohmann::json &);
};
