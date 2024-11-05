#include "pitched_note/PitchedNote.hpp"

#include <QObject>
#include <QtGlobal>
#include <nlohmann/json.hpp>

#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "justly/PitchedNoteColumn.hpp"
#include "named/Named.hpp"
#include "other/other.hpp"
#include "rational/AbstractRational.hpp"
#include "rational/Rational.hpp"

PitchedNote::PitchedNote(const nlohmann::json &json_note)
    : instrument_pointer(json_field_to_named_pointer(
          Instrument::get_all_nameds(), json_note, "instrument")),
      interval(json_field_to_interval(json_note)),
      beats(json_field_to_rational(json_note, "beats")),
      velocity_ratio(json_field_to_rational(json_note, "velocity_ratio")),
      words(json_field_to_words(json_note)) {}

auto PitchedNote::get_column_name(int column_number) -> QString {
  switch (column_number) {
  case pitched_note_instrument_column:
    return QObject::tr("Instrument");
  case pitched_note_interval_column:
    return QObject::tr("Interval");
  case pitched_note_beats_column:
    return QObject::tr("Beats");
  case pitched_note_velocity_ratio_column:
    return QObject::tr("Velocity ratio");
  case pitched_note_words_column:
    return QObject::tr("Words");
  default:
    Q_ASSERT(false);
    return {};
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

void PitchedNote::set_data_directly(int column, const QVariant &new_value) {
  switch (column) {
  case pitched_note_instrument_column:
    instrument_pointer = variant_to_instrument(new_value);
    break;
  case pitched_note_interval_column:
    interval = variant_to_interval(new_value);
    break;
  case pitched_note_beats_column:
    beats = variant_to_rational(new_value);
    break;
  case pitched_note_velocity_ratio_column:
    velocity_ratio = variant_to_rational(new_value);
    break;
  case pitched_note_words_column:
    words = variant_to_string(new_value);
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
  auto json_note = nlohmann::json::object();
  add_named_to_json(json_note, instrument_pointer, "instrument");
  add_abstract_rational_to_json(json_note, interval, "interval");
  add_abstract_rational_to_json(json_note, beats, "beats");
  add_abstract_rational_to_json(json_note, velocity_ratio, "velocity_ratio");
  add_words_to_json(json_note, words);
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
      "the pitched_notes",
      get_object_schema(
          "a pitched_note",
          nlohmann::json(
              {{"instrument", get_named_schema(Instrument::get_all_nameds(),
                                               "the instrument")},
               {"interval", get_interval_schema()},
               {"beats", get_rational_schema("the number of beats")},
               {"velocity_ratio", get_rational_schema("velocity ratio")},
               {"words", get_words_schema()}})));
}
