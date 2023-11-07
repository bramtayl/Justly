#pragma once

#include <nlohmann/json_fwd.hpp>  // for json
#include <string>

#include "justly/Interval.h"  // for Interval

const auto MINIMUM_BEATS = 1;
const auto DEFAULT_BEATS = 1;
const auto MAXIMUM_BEATS = 199;

const auto MINIMUM_VOLUME_PERCENT = 1;
const auto DEFAULT_VOLUME_PERCENT = 100.0;
const auto MAXIMUM_VOLUME_PERCENT = 400;

const auto MINIMUM_TEMPO_PERCENT = 1;
const auto DEFAULT_TEMPO_PERCENT = 100.0;
const auto MAXIMUM_TEMPO_PERCENT = 400;

const auto DEFAULT_WORDS = "";

struct Instrument;

struct NoteChord {
  Interval interval;
  int beats;
  double volume_percent;
  double tempo_percent;
  std::string words;
  const Instrument *instrument_pointer;

  NoteChord();
  explicit NoteChord(const nlohmann::json &);
  virtual ~NoteChord() = default;

  [[nodiscard]] virtual auto to_json() const -> nlohmann::json;
  [[nodiscard]] virtual auto symbol_for() const -> std::string = 0;

 protected:
  [[nodiscard]] static auto get_instrument_schema() -> nlohmann::json &;
  [[nodiscard]] static auto get_words_schema() -> nlohmann::json &;
  [[nodiscard]] static auto get_volume_percent_schema() -> nlohmann::json &;
  [[nodiscard]] static auto get_tempo_percent_schema() -> nlohmann::json &;
  [[nodiscard]] static auto get_beats_schema() -> nlohmann::json &;
};
