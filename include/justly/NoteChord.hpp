#pragma once

#include <QSize>              // for QSize
#include <QVariant>           // for QVariant
#include <nlohmann/json.hpp>  // for json
#include <string>             // for string

#include "justly/Instrument.hpp"
#include "justly/Interval.hpp"          // for Interval
#include "justly/NoteChordField.hpp"    // for NoteChordField
#include "justly/Rational.hpp"          // for Rational
#include "justly/public_constants.hpp"  // for JUSTLY_EXPORT

const auto MAX_BEATS = 199;

[[nodiscard]] auto get_words_size() -> QSize;

struct JUSTLY_EXPORT NoteChord {
  const Instrument* instrument_pointer;
  Interval interval;
  Rational beats;
  Rational volume_ratio;
  Rational tempo_ratio;
  std::string words;

  NoteChord();
  explicit NoteChord(const nlohmann::json& json_note_chord);
  virtual ~NoteChord() = default;

  [[nodiscard]] virtual auto symbol() const -> std::string = 0;
  [[nodiscard]] virtual auto json() const -> nlohmann::json;

  [[nodiscard]] auto data(NoteChordField note_chord_field, int role) const
      -> QVariant;
  void setData(NoteChordField note_chord_field, const QVariant& new_value);
};

[[nodiscard]] auto get_note_chord_fields_schema() -> const nlohmann::json&;
