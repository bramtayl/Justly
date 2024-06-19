#pragma once

#include <nlohmann/json_fwd.hpp>  // for json
#include <string>

#include "justly/global.h"
#include "justly/Interval.h"  // for Interval

const auto MIN_BEATS = 1;
const auto DEFAULT_BEATS = 1;
const auto MAX_BEATS = 199;

const auto MIN_VOLUME_PERCENT = 1;
const auto DEFAULT_VOLUME_PERCENT = 100.0;
const auto MAX_VOLUME_PERCENT = 400;

const auto MIN_TEMPO_PERCENT = 1;
const auto DEFAULT_TEMPO_PERCENT = 100.0;
const auto MAX_TEMPO_PERCENT = 400;

const auto DEFAULT_WORDS = "";

struct Instrument;

struct JUSTLY_EXPORT NoteChord {
  Interval interval;
  int beats;
  double volume_percent;
  double tempo_percent;
  std::string words;
  const Instrument *instrument_pointer;

  NoteChord();
  explicit NoteChord(const nlohmann::json &);
  virtual ~NoteChord() = default;

  [[nodiscard]] virtual auto json() const -> nlohmann::json;
  [[nodiscard]] virtual auto symbol() const -> std::string = 0;
};
