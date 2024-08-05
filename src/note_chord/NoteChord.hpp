#pragma once

#include <QString>
#include <QVariant>
#include <nlohmann/json.hpp>

#include "justly/Interval.hpp"
#include "justly/NoteChordColumn.hpp"
#include "justly/Rational.hpp"

class Instrument;

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

  [[nodiscard]] auto data(NoteChordColumn note_chord_column) const -> QVariant;
  void setData(NoteChordColumn note_chord_column, const QVariant &new_value);
  void replace_cells(NoteChordColumn left_field, NoteChordColumn right_field,
                     const NoteChord &new_note_chord);
};
