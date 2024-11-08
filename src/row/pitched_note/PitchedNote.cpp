#include "row/pitched_note/PitchedNote.hpp"

#include <QString>
#include <QtGlobal>
#include <nlohmann/json.hpp>

#include "abstract_rational/AbstractRational.hpp"
#include "abstract_rational/interval/Interval.hpp"
#include "abstract_rational/rational/Rational.hpp"
#include "justly/PitchedNoteColumn.hpp"
#include "named/Named.hpp"
#include "named/program/instrument/Instrument.hpp"
#include "other/other.hpp"

PitchedNote::PitchedNote(const nlohmann::json &json_note)
    : Row(json_note),
      instrument_pointer(
          json_field_to_named_pointer<Instrument>(json_note, "instrument")),
      interval(
          json_field_to_abstract_rational<Interval>(json_note, "interval")) {}

auto PitchedNote::get_column_name(int column_number) -> const char* {
  switch (column_number) {
  case pitched_note_instrument_column:
    return "Instrument";
  case pitched_note_interval_column:
    return "Interval";
  case pitched_note_beats_column:
    return "Beats";
  case pitched_note_velocity_ratio_column:
    return "Velocity ratio";
  case pitched_note_words_column:
    return "Words";
  default:
    Q_ASSERT(false);
    return "";
  }
}

auto PitchedNote::get_number_of_columns() -> int {
  return number_of_pitched_note_columns;
}

auto PitchedNote::get_data(int column_number) const -> QVariant {
  switch (column_number) {
  case pitched_note_instrument_column:
    return QVariant::fromValue(instrument_pointer);
  case pitched_note_interval_column:
    return QVariant::fromValue(interval);
  case pitched_note_beats_column:
    return QVariant::fromValue(beats);
  case pitched_note_velocity_ratio_column:
    return QVariant::fromValue(velocity_ratio);
  case pitched_note_words_column:
    return words;
  default:
    Q_ASSERT(false);
    return {};
  }
}

void PitchedNote::set_data(int column, const QVariant &new_value) {
  switch (column) {
  case pitched_note_instrument_column:
    instrument_pointer = variant_to<const Instrument *>(new_value);
    break;
  case pitched_note_interval_column:
    interval = variant_to<Interval>(new_value);
    break;
  case pitched_note_beats_column:
    beats = variant_to<Rational>(new_value);
    break;
  case pitched_note_velocity_ratio_column:
    velocity_ratio = variant_to<Rational>(new_value);
    break;
  case pitched_note_words_column:
    words = variant_to<QString>(new_value);
    break;
  default:
    Q_ASSERT(false);
  }
};

void PitchedNote::copy_columns_from(const PitchedNote &template_row,
                                    int left_column, int right_column) {
  for (auto note_column = left_column; note_column <= right_column;
       note_column++) {
    switch (note_column) {
    case pitched_note_instrument_column:
      instrument_pointer = template_row.instrument_pointer;
      break;
    case pitched_note_interval_column:
      interval = template_row.interval;
      break;
    case pitched_note_beats_column:
      beats = template_row.beats;
      break;
    case pitched_note_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    case pitched_note_words_column:
      words = template_row.words;
      break;
    default:
      Q_ASSERT(false);
    }
  }
};

[[nodiscard]] auto PitchedNote::to_json() const -> nlohmann::json {
  auto json_note = Row::to_json();;
  add_named_to_json(json_note, instrument_pointer, "instrument");
  add_abstract_rational_to_json(json_note, interval, "interval");
  return json_note;
}

[[nodiscard]] auto
PitchedNote::columns_to_json(int left_column,
                             int right_column) const -> nlohmann::json {
  auto json_note = nlohmann::json::object();
  for (auto note_column = left_column; note_column <= right_column;
       note_column++) {
    switch (note_column) {
    case pitched_note_instrument_column:
      add_named_to_json(json_note, instrument_pointer, "instrument");
      break;
    case pitched_note_interval_column:
      add_abstract_rational_to_json(json_note, interval, "interval");
      break;
    case pitched_note_beats_column:
      add_abstract_rational_to_json(json_note, beats, "beats");
      break;
    case pitched_note_velocity_ratio_column:
      add_abstract_rational_to_json(json_note, velocity_ratio,
                                    "velocity_ratio");
      break;
    case pitched_note_words_column:
      add_words_to_json(json_note, words);
      break;
    default:
      Q_ASSERT(false);
      break;
    }
  }
  return json_note;
}

auto get_pitched_notes_schema() -> nlohmann::json {
  return get_array_schema(
      "the pitched notes",
      get_object_schema(
          "a pitched_note",
          nlohmann::json(
              {{"instrument", get_named_schema<Instrument>("the instrument")},
               {"interval", get_interval_schema()},
               {"beats", get_rational_schema("the number of beats")},
               {"velocity_ratio", get_rational_schema("velocity ratio")},
               {"words", get_words_schema()}})));
}