#pragma once

#include <QString>
#include <QVariant>
#include <nlohmann/json.hpp>

#include "justly/Interval.hpp"
#include "justly/JUSTLY_EXPORT.hpp"
#include "justly/NoteChordField.hpp"
#include "justly/Rational.hpp"

struct Instrument;

[[nodiscard]] auto get_note_chord_fields_schema() -> const nlohmann::json &;

struct JUSTLY_EXPORT NoteChord {
  const Instrument *instrument_pointer;
  Interval interval;
  Rational beats;
  Rational volume_ratio;
  Rational tempo_ratio;
  QString words;

  NoteChord();
  explicit NoteChord(const nlohmann::json &json_note_chord);
  virtual ~NoteChord() = default;

  [[nodiscard]] virtual auto symbol() const -> QString;
  [[nodiscard]] virtual auto json() const -> nlohmann::json;

  [[nodiscard]] auto data(NoteChordField note_chord_field,
                          int role) const -> QVariant;
  void setData(NoteChordField note_chord_field, const QVariant &new_value);
  void replace_cells(NoteChordField left_field, NoteChordField right_field,
                     const NoteChord &new_note_chord);
};
