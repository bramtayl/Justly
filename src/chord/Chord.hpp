#pragma once

#include <QList>
#include <QString>
#include <QVariant>
#include <nlohmann/json.hpp>

#include "interval/Interval.hpp"
#include "pitched_note/PitchedNote.hpp" // IWYU pragma: keep
#include "rational/Rational.hpp"
#include "rows/Row.hpp"
#include "unpitched_note/UnpitchedNote.hpp" // IWYU pragma: keep

struct Instrument;
struct PercussionInstrument;
struct PercussionSet;

struct Chord : public Row {
  const Instrument *instrument_pointer = nullptr;
  const PercussionSet *percussion_set_pointer = nullptr;
  const PercussionInstrument *percussion_instrument_pointer = nullptr;
  Interval interval;
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;
  QString words;
  QList<PitchedNote> pitched_notes;
  QList<UnpitchedNote> unpitched_notes;

  Chord() = default;
  explicit Chord(const nlohmann::json &json_chord);

  [[nodiscard]] static auto get_column_name(int column_number) -> QString;
  [[nodiscard]] static auto get_number_of_columns() -> int;

  [[nodiscard]] auto get_data(int column_number) const -> QVariant override;
  void set_data_directly(int column, const QVariant &new_value) override;

  void copy_columns_from(const Chord &template_row, int left_column,
                         int right_column);
  [[nodiscard]] auto to_json() const -> nlohmann::json override;
  [[nodiscard]] auto columns_to_json(int left_column, int right_column) const
      -> nlohmann::json override;
};

[[nodiscard]] auto get_chords_schema() -> nlohmann::json;
