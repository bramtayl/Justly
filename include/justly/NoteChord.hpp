#pragma once

#include <nlohmann/json_fwd.hpp>  // for json
#include <string>

#include "justly/global.hpp"
#include "justly/Interval.hpp"  // for Interval
#include "justly/Rational.hpp"

const auto MIN_BEATS = 1;
const auto DEFAULT_BEATS = 1;
const auto MAX_BEATS = 199;

const auto DEFAULT_WORDS = "";

struct Instrument;

struct JUSTLY_EXPORT NoteChord {
  Interval interval;
  int beats;
  Rational volume_ratio;
  Rational tempo_ratio;
  std::string words;
  const Instrument *instrument_pointer;

  NoteChord();
  explicit NoteChord(const nlohmann::json & json_note_chord);
  virtual ~NoteChord() = default;

  [[nodiscard]] virtual auto symbol() const -> std::string = 0;
  [[nodiscard]] virtual auto json() const -> nlohmann::json;
};
