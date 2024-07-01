#pragma once

#include <nlohmann/json_fwd.hpp>  // for json
#include <string>

#include "justly/public_constants.hpp"
#include "justly/Interval.hpp"  // for Interval
#include "justly/Rational.hpp"

const auto MAX_BEATS = 199;

struct Instrument;

struct JUSTLY_EXPORT NoteChord {
  const Instrument *instrument_pointer;
  Interval interval;
  Rational beats;
  Rational volume_ratio;
  Rational tempo_ratio;
  std::string words;

  NoteChord();
  explicit NoteChord(const nlohmann::json & json_note_chord);
  virtual ~NoteChord() = default;

  [[nodiscard]] virtual auto symbol() const -> std::string = 0;
  [[nodiscard]] virtual auto json() const -> nlohmann::json;
};

auto JUSTLY_EXPORT get_note_chord_fields_schema() -> const nlohmann::json&;
