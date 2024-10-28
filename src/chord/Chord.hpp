#pragma once

#include <QList>
#include <QString>
#include <QVariant>
#include <nlohmann/json.hpp>

#include "interval/Interval.hpp"
#include "justly/ChordColumn.hpp"
#include "note/Note.hpp"             // IWYU pragma: keep
#include "percussion/Percussion.hpp" // IWYU pragma: keep
#include "rational/Rational.hpp"
#include "rows/Row.hpp"

struct Instrument;
struct PercussionInstrument;
struct PercussionSet;

namespace nlohmann::json_schema {
class json_validator;
} // namespace nlohmann::json_schema

const auto NUMBER_OF_CHORD_COLUMNS = 10;

[[nodiscard]] auto to_chord_column(int column) -> ChordColumn;

struct Chord : public Row {
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

  void from_json(const nlohmann::json &json_chord) override;

  [[nodiscard]] auto get_data(int column_number) const -> QVariant override;
  void set_data_directly(int column, const QVariant &new_value) override;

  void copy_columns_from(const Chord &template_row, int left_column,
                         int right_column);
  [[nodiscard]] auto to_json(int left_column,
                             int right_column) const -> nlohmann::json override;
};

[[nodiscard]] auto get_chords_schema() -> nlohmann::json;
[[nodiscard]] auto
get_chords_cells_validator() -> const nlohmann::json_schema::json_validator &;
