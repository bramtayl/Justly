#pragma once

#include <memory>                 // for unique_ptr
#include <nlohmann/json_fwd.hpp>  // for json
#include <vector>                 // for vector

#include "justly/Chord.h"  // for Chord

struct Instrument;

const auto MIN_STARTING_KEY = 60;
const auto MAX_STARTING_KEY = 440;

const auto MIN_STARTING_VOLUME = 1;
const auto MAX_STARTING_VOLUME = 100;

const auto MIN_STARTING_TEMPO = 100;
const auto MAX_STARTING_TEMPO = 800;

struct Song {
  double starting_key;
  double starting_volume;
  double starting_tempo;
  const Instrument *starting_instrument_pointer;
  std::vector<std::unique_ptr<Chord>> chord_pointers;

  Song();

  [[nodiscard]] auto json() const -> nlohmann::json;

  void load(const nlohmann::json &);
};
