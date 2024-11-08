#pragma once

#include <QList>
#include <QVariant>
#include <nlohmann/json.hpp>

#include "abstract_rational/interval/Interval.hpp"
#include "abstract_rational/rational/Rational.hpp"
#include "row/Row.hpp"
#include "row/note/pitched_note/PitchedNote.hpp" // IWYU pragma: keep
#include "row/note/unpitched_note/UnpitchedNote.hpp" // IWYU pragma: keep

struct Instrument;
struct PercussionInstrument;
struct PercussionSet;

struct Chord : public Row {
  const Instrument *instrument_pointer = nullptr;
  const PercussionSet *percussion_set_pointer = nullptr;
  const PercussionInstrument *percussion_instrument_pointer = nullptr;
  Interval interval;
  Rational tempo_ratio;
  QList<PitchedNote> pitched_notes;
  QList<UnpitchedNote> unpitched_notes;

  Chord() = default;
  explicit Chord(const nlohmann::json &json_chord);

  [[nodiscard]] static auto get_column_name(int column_number) -> const char*;
  [[nodiscard]] static auto get_number_of_columns() -> int;
  [[nodiscard]] static auto is_column_editable(int column_number) -> bool;

  [[nodiscard]] auto get_data(int column_number) const -> QVariant override;
  void set_data(int column, const QVariant &new_value) override;

  void copy_columns_from(const Chord &template_row, int left_column,
                         int right_column);
  [[nodiscard]] auto to_json() const -> nlohmann::json override;
  [[nodiscard]] auto columns_to_json(int left_column, int right_column) const
      -> nlohmann::json override;
};

