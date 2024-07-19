#pragma once

#include <QString>
#include <QVariant>
#include <nlohmann/json.hpp>
#include <string>

#include "justly/DataType.hpp"
#include "justly/Instrument.hpp"
#include "justly/Interval.hpp"
#include "justly/JUSTLY_EXPORT.hpp"
#include "justly/NoteChordField.hpp"
#include "justly/Rational.hpp"

[[nodiscard]] auto get_data_type(NoteChordField note_chord_field) -> DataType;
[[nodiscard]] auto get_mime_type(DataType data_type) -> QString;

void copy_json(const nlohmann::json &copied, const QString &mime_type);

[[nodiscard]] auto get_words_schema() -> const nlohmann::json &;
[[nodiscard]] auto get_note_chord_fields_schema() -> const nlohmann::json &;

void JUSTLY_EXPORT copy_text(const std::string &text, const QString &mime_type);

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

  [[nodiscard]] virtual auto symbol() const -> QString = 0;
  [[nodiscard]] virtual auto json() const -> nlohmann::json;

  [[nodiscard]] auto data(NoteChordField note_chord_field,
                          int role) const -> QVariant;
  void setData(NoteChordField note_chord_field, const QVariant &new_value);
  void copy_cell(NoteChordField note_chord_field) const;
};
