#pragma once

#include <QString>
#include <nlohmann/json.hpp>
#include <variant>

#include "interval/Interval.hpp"
#include "rational/Rational.hpp"

struct Instrument;
struct Percussion;

// json_to_interval
// rational_is_default
// rational_to_json
// json_to_rational

struct NoteChord {
  const Instrument *instrument_pointer = nullptr;
  std::variant<Interval, const Percussion *> interval_or_percussion_pointer = Interval();
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;
  QString words;

  NoteChord() = default;
  explicit NoteChord(const nlohmann::json &json_note_chord);
  virtual ~NoteChord() = default;

  [[nodiscard]] virtual auto is_chord() const -> bool;
};

[[nodiscard]] auto
note_chord_to_json(const NoteChord *note_chord_pointer) -> nlohmann::json;
