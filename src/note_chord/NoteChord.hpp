#pragma once

#include <QString>
#include <nlohmann/json.hpp>

#include "cell_values/Interval.hpp"
#include "cell_values/Rational.hpp"

struct Instrument;

// json_to_interval
// rational_is_default
// rational_to_json
// json_to_rational

struct NoteChord {
  const Instrument *instrument_pointer;
  Interval interval;
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;
  QString words;

  NoteChord();
  explicit NoteChord(const nlohmann::json &json_note_chord);
  virtual ~NoteChord() = default;

  [[nodiscard]] virtual auto is_chord() const -> bool;
  [[nodiscard]] virtual auto get_symbol() const -> QString;
  [[nodiscard]] virtual auto to_json() const -> nlohmann::json;
};
