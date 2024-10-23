#pragma once

#include <QList>
#include <QString>
#include <nlohmann/json.hpp>

#include "interval/Interval.hpp"
#include "note/Note.hpp"
#include "percussion/Percussion.hpp"
#include "rational/Rational.hpp"

struct Instrument;
struct PercussionInstrument;
struct PercussionSet;

namespace nlohmann::json_schema {
class json_validator;
} // namespace nlohmann::json_schema

struct Chord {
  QList<Note> notes;
  QList<Percussion> percussions;

  const Instrument *instrument_pointer = nullptr;
  const PercussionSet *percussion_set_pointer = nullptr;
  const PercussionInstrument *percussion_instrument_pointer = nullptr;

  Interval interval;
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;
  QString words;

  Chord() = default;
  explicit Chord(const nlohmann::json &json_chord);
  
  void copy_columns_from(const Chord& template_chord, int left_column, int right_column);
  [[nodiscard]] auto to_json(int left_column,
                             int right_column) const -> nlohmann::json;
};

[[nodiscard]] auto get_chords_schema() -> nlohmann::json;
[[nodiscard]] auto
get_chords_cells_validator() -> const nlohmann::json_schema::json_validator &;
