#pragma once

#include <memory>                 // for unique_ptr
#include <nlohmann/json_fwd.hpp>  // for json
#include <vector>                 // for vector

#include "justly/Chord.h"  // for Chord

struct Instrument;

const auto MINIMUM_STARTING_KEY = 60;
const auto MAXIMUM_STARTING_KEY = 440;

const auto MINIMUM_STARTING_VOLUME = 1;
const auto MAXIMUM_STARTING_VOLUME = 100;

const auto MINIMUM_STARTING_TEMPO = 100;
const auto MAXIMUM_STARTING_TEMPO = 800;

struct Song {
  double starting_key;
  double starting_volume;
  double starting_tempo;
  const Instrument *starting_instrument_pointer;
  std::vector<std::unique_ptr<Chord>> chord_pointers;

  Song();

  [[nodiscard]] auto to_json() const -> nlohmann::json;
  [[nodiscard]] static auto verify_json(const nlohmann::json &) -> bool;

  void load_from(const nlohmann::json &);
  [[nodiscard]] auto children_to_json(int, int) const -> nlohmann::json;
  void insert_empty_chilren(int, int);
  void remove_children(int, int);
  void insert_json_chilren(int, const nlohmann::json &);
};
