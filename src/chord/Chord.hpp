#pragma once

#include <QList>
#include <QString>
#include <QVariant>
#include <nlohmann/json.hpp>

#include "interval/Interval.hpp"
#include "justly/ChordColumn.hpp"
#include "rational/Rational.hpp"

struct Instrument;
struct Note;
struct Percussion;
struct PercussionInstrument;
struct PercussionSet;

namespace nlohmann::json_schema {
class json_validator;
} // namespace nlohmann::json_schema

const auto NUMBER_OF_CHORD_COLUMNS = 10;

[[nodiscard]] auto to_chord_column(int column) -> ChordColumn;

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

  [[nodiscard]] auto get_data(int column_number) const -> QVariant;
  void set_data_directly(int column, const QVariant &new_value);

  void copy_columns_from(const Chord &template_chord, int left_column,
                         int right_column);
  [[nodiscard]] auto to_json(int left_column,
                             int right_column) const -> nlohmann::json;
};

[[nodiscard]] auto get_chords_schema() -> nlohmann::json;
[[nodiscard]] auto
get_chords_cells_validator() -> const nlohmann::json_schema::json_validator &;
